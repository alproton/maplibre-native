
#include <mbgl/gfx/custom_puck.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/logging.hpp>
#include <numbers>
#include <mutex>

namespace mbgl {
namespace gfx {

namespace {

CustomPuckScreenQuad customPuckScreenQuad(
    const ScreenCoordinate& screenCoord, double dx, double dy, double cosPitch, double cosBearing, double sinBearing) {
    CustomPuckScreenQuad quad = {
        ScreenCoordinate{-1, -1},
        ScreenCoordinate{1, -1},
        ScreenCoordinate{-1, 1},
        ScreenCoordinate{1, 1},
    };

    for (auto& v : quad) {
        double x = v.x * cosBearing - v.y * sinBearing;
        double y = v.x * sinBearing + v.y * cosBearing;
        y *= cosPitch;
        v.x = screenCoord.x + x * dx;
        v.y = screenCoord.y + y * dy;
    }

    return quad;
}

CustomPuckSampledIcon customPuckSampledIcon(const CustomPuckIconStyle& style,
                                            float time,
                                            const ScreenCoordinate& screenCoord,
                                            double dx,
                                            double dy,
                                            double cosPitch,
                                            double cosBearing,
                                            double sinBearing) {
    CustomPuckSampledIcon icon;
    icon.name = style.name;
    float opacity = style.sampleOpacity(time);
    float scale = style.sampleScale(time);
    dx *= style.mirrorX.value_or(false) ? -scale : scale;
    dy *= style.mirrorY.value_or(false) ? -scale : scale;
    icon.color = style.color.value_or(Color::white()) * opacity;
    icon.quad = customPuckScreenQuad(screenCoord, dx, dy, cosPitch, cosBearing, sinBearing);
    return icon;
}

} // namespace

void CustomPuck::draw(const TransformState& transform) {
    const auto& state = getState();
    if (!state.enabled) {
        return;
    }

    if (state.cameraTracking != isCameraTracking) {
        isCameraTracking = state.cameraTracking;
        Log::Warning(Event::General,
                     std::string("Puck displayed with camera tracking ") + (isCameraTracking ? "enabled" : "disabled"));
    }

    float bearing = static_cast<float>(-state.bearing * std::numbers::pi / 180.0 - transform.getBearing());
    auto latlon = transform.getLatLng();
    if (!state.cameraTracking) {
        latlon = LatLng(state.lat, state.lon);
    }

    const auto screenSize = transform.getSize();
    auto screenCoord = transform.latLngToScreenCoordinate(latlon);
    auto pitch = transform.getPitch();
    screenCoord.x = screenCoord.x / screenSize.width;
    screenCoord.y = screenCoord.y / screenSize.height;
    screenCoord.x = screenCoord.x * 2 - 1;
    screenCoord.y = screenCoord.y * 2 - 1;

    double cosPitch = std::cos(pitch);
    double cosBearing = std::cos(bearing);
    double sinBearing = std::sin(bearing);

    constexpr float defaultIconPixelSize = 128;
    float iconPixelSize = state.iconScale * defaultIconPixelSize;
    double dx = iconPixelSize / static_cast<double>(screenSize.width);
    double dy = iconPixelSize / static_cast<double>(screenSize.height);

    CustomPuckIconsStyle iconsStyle;
    CustomPuckSampledStyle sampledStyle;
    {
        std::lock_guard<std::mutex> lock(styleMutex);
        if (style.variants.empty() || variantName.empty() || iconStateName.empty()) {
            return; // Style not set yet
        }
        if (!style.icons.empty()) {
            epochTime = std::chrono::steady_clock::now();
        }
        sampledStyle.icons = std::move(style.icons); // only needed once to load the textures
        style.icons.clear();
        assert(style.variants.count(variantName) > 0);
        assert(style.variants[variantName].count(iconStateName) > 0);
        iconsStyle = style.variants[variantName][iconStateName];
    }
    if (!iconsStyle.icon1 && !iconsStyle.icon2) {
        return; // Style not set yet
    }

    auto deltaTime = std::chrono::duration<float>(std::chrono::steady_clock::now() - epochTime).count();

    if (iconsStyle.icon1) {
        sampledStyle.icon1 = customPuckSampledIcon(
            *iconsStyle.icon1, deltaTime, screenCoord, dx, dy, cosPitch, cosBearing, sinBearing);
    }
    if (iconsStyle.icon2) {
        sampledStyle.icon2 = customPuckSampledIcon(
            *iconsStyle.icon2, deltaTime, screenCoord, dx, dy, cosPitch, cosBearing, sinBearing);
    }

    drawImpl(sampledStyle);
}

void CustomPuck::setAssetPath(const std::string& path) {
    assetPath = path;
}

void CustomPuck::setPuckStyle(const std::string& style_file_path) {
    auto json = readFile(style_file_path);

    std::lock_guard<std::mutex> lock(styleMutex);
    style = parseCustomPuckStyle(json);
}

void CustomPuck::setPuckVariant(std::string variant) {
    std::lock_guard<std::mutex> lock(styleMutex);
    variantName = std::move(variant);
}

void CustomPuck::setPuckIconState(std::string state) {
    std::lock_guard<std::mutex> lock(styleMutex);
    iconStateName = std::move(state);
    epochTime = std::chrono::steady_clock::now();
}

#ifdef __ANDROID__
void CustomPuck::setAssetManager(AAssetManager* asset_manager) {
    std::lock_guard<std::mutex> lock(styleMutex);
    android_asset_manager = asset_manager;
}

AAssetManager* CustomPuck::getAssetManager() {
    std::lock_guard<std::mutex> lock(styleMutex);
    assert(android_asset_manager != nullptr);
    return android_asset_manager;
}
#endif

std::string CustomPuck::readFile(const std::string& path) const {
#ifdef __ANDROID__
    if (!android_asset_manager) {
        throw std::runtime_error("Missing asset manager");
    }
    AAsset* asset = AAssetManager_open(android_asset_manager, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        throw std::runtime_error("Failed to open asset: " + path);
    }
    auto size = AAsset_getLength(asset);
    auto data = AAsset_getBuffer(asset);
    if (size == 0) {
        throw std::runtime_error("Empty asset: " + path);
    }
    if (!data) {
        throw std::runtime_error("Failed to read asset: " + path);
    }
    std::string result(reinterpret_cast<const char*>(data), size);
    AAsset_close(asset);
    return result;
#else
    return util::read_file(assetPath + path);
#endif
}

} // namespace gfx
} // namespace mbgl
