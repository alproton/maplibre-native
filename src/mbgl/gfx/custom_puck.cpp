
#include <mbgl/gfx/custom_puck.hpp>
#include <numbers>
#include <mutex>
#include <random>
#include <mbgl/util/tile_coordinate.hpp>
#include <mbgl/util/logging.hpp>

#if ANDROID
#include <sys/system_properties.h>
static std::string androidSysProp(const char* key) {
    assert(strlen(key) < PROP_NAME_MAX);
    if (__system_property_find(key) == nullptr) {
        return "";
    }
    char prop[PROP_VALUE_MAX + 1];
    __system_property_get(key, prop);
    return prop;
}
static bool isDebugChargersEnabled() {
    auto prop = androidSysProp("rivian.navigation-debug-chargers");
    return prop == "1" || prop == "true" || prop == "TRUE" || prop == "True";
}
#else
static bool isDebugChargersEnabled() {
    return true;
}
#endif

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

void CustomPuck::debugChargers(const TransformState& transform) {
    constexpr int pointCount = 10000;
    constexpr int gridSize = 200;

    if (!isDebugChargersEnabled()) {
        return;
    }

    std::mt19937 gen(static_cast<uint32_t>(transform.getLatLng().latitude()) ^
                     static_cast<uint32_t>(transform.getLatLng().longitude()) ^
                     static_cast<uint32_t>(transform.getIntegerZoom()));
    auto random = [&](double min, double max) {
        std::uniform_real_distribution<double> dis(min, max);
        return dis(gen);
    };

    auto screenSize = transform.getSize();

    auto c1 = transform.screenCoordinateToLatLng(ScreenCoordinate{0, 0});
    auto c2 = transform.screenCoordinateToLatLng(ScreenCoordinate{(double)screenSize.width, (double)screenSize.height});

    double minLat = std::min(c1.latitude(), c2.latitude());
    double maxLat = std::max(c1.latitude(), c2.latitude());
    double minLon = std::min(c1.longitude(), c2.longitude());
    double maxLon = std::max(c1.longitude(), c2.longitude());

    std::vector<char> grid(gridSize * gridSize, 0);
    std::vector<float> vertexBuffer;
    vertexBuffer.reserve(pointCount * 2);
    for (int p = 0; p < pointCount * 3; p++) {
        double lat = random(minLat, maxLat);
        double lon = random(minLon, maxLon);
        auto latlon = LatLng(lat, lon);
        auto screenCoord = transform.latLngToScreenCoordinate(latlon);
        screenCoord.x = screenCoord.x / screenSize.width;
        screenCoord.y = screenCoord.y / screenSize.height;
        screenCoord.x = screenCoord.x * 2 - 1;
        screenCoord.y = screenCoord.y * 2 - 1;
        if (std::abs(screenCoord.x) < 0 || std::abs(screenCoord.x) > 1 || std::abs(screenCoord.y) < 0 ||
            std::abs(screenCoord.y) > 1) {
            continue;
        }
        int x = static_cast<int>((screenCoord.x + 1) / 2 * gridSize);
        int y = static_cast<int>((screenCoord.y + 1) / 2 * gridSize);
        if (x < 0 || x >= gridSize || y < 0 || y >= gridSize) {
            continue;
        }
        bool collides = false;
        for (int yy = -1; yy <= 1; ++yy) {
            for (int xx = -1; xx <= 1; ++xx) {
                int i = std::max(0, std::min(y + yy, gridSize - 1));
                int j = std::max(0, std::min(x + xx, gridSize - 1));
                if (grid[i * gridSize + j] > 0) {
                    collides = true;
                }
            }
        }
        if (collides) {
            continue;
        }
        grid[y * gridSize + x] = 1;
        vertexBuffer.push_back(static_cast<float>(screenCoord.x));
        vertexBuffer.push_back(static_cast<float>(screenCoord.y));
        if (vertexBuffer.size() >= pointCount * 2) {
            break;
        }
    }

    constexpr double defaultIconPixelSize = 12;
    double ratio = static_cast<double>(screenSize.width) / static_cast<double>(screenSize.height);
    double dx = defaultIconPixelSize / static_cast<double>(screenSize.width);
    double dy = defaultIconPixelSize * ratio / static_cast<double>(screenSize.width);

    drawChargerImpl(vertexBuffer, static_cast<float>(dx), static_cast<float>(dy));
}

} // namespace gfx
} // namespace mbgl
