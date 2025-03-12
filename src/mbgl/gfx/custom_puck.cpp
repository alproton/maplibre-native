
#include <mbgl/gfx/custom_puck.hpp>
#include <numbers>
#include <mutex>

namespace mbgl {
namespace gfx {

namespace {

PremultipliedImage& puckBitmap() {
    static PremultipliedImage image{};
    return image;
}

std::mutex& puckBitmapMutex() {
    static std::mutex mutex;
    return mutex;
}

} // namespace

void CustomPuck::draw(const TransformState& transform) {
    const auto& state = getState();
    if (!state.enabled) {
        return;
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

    ScreenQuad quad = {
        ScreenCoordinate{-1, -1},
        ScreenCoordinate{1, -1},
        ScreenCoordinate{-1, 1},
        ScreenCoordinate{1, 1},
    };

    double cosPitch = std::cos(pitch);
    double cosBearing = std::cos(bearing);
    double sinBearing = std::sin(bearing);

    constexpr float defaultIconPixelSize = 128;
    float iconPixelSize = state.iconScale * defaultIconPixelSize;
    double dx = iconPixelSize / static_cast<double>(screenSize.width);
    double dy = iconPixelSize / static_cast<double>(screenSize.height);

    for (auto& v : quad) {
        double x = v.x * cosBearing - v.y * sinBearing;
        double y = v.x * sinBearing + v.y * cosBearing;
        y *= cosPitch;
        v.x = screenCoord.x + x * dx;
        v.y = screenCoord.y + y * dy;
    }

    drawImpl(quad);
}

void setPuckBitmap(const PremultipliedImage& src) {
    assert(src.valid());
    std::lock_guard<std::mutex> lock(puckBitmapMutex());
    auto& dst = puckBitmap();
    if (dst.valid()) {
        return;
    }
    dst.size = src.size;
    dst.data = std::make_unique<uint8_t[]>(src.bytes());
    std::memcpy(dst.data.get(), src.data.get(), src.bytes());
}

const PremultipliedImage& getPuckBitmap() {
    std::lock_guard<std::mutex> lock(puckBitmapMutex());
    return puckBitmap();
}

} // namespace gfx
} // namespace mbgl
