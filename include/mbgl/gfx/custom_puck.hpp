#pragma once

#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/image.hpp>
#include <array>
#include <memory>
#include <string>

namespace mbgl {
namespace gfx {

struct CustomPuckState {
    double lat = 0;
    double lon = 0;
    double bearing = 0;
    float iconScale = 1.f;
    bool cameraTracking = false;
    bool enabled = false;
};

class CustomPuck {
public:
    virtual ~CustomPuck() noexcept {};

    void draw(const TransformState& transform);

    void setPuckBitmap(const PremultipliedImage& src);

    PremultipliedImage getPuckBitmap();

    void setPuckStyle(const std::string& style_file_path);

protected:
    using ScreenQuad = std::array<ScreenCoordinate, 4>;

    virtual void drawImpl(const ScreenQuad&) = 0;

    virtual CustomPuckState getState() = 0;

private:
    PremultipliedImage bitmap{};
    // The puck icon bitmap is updated in the UI thread and is used in the rendering thread
    // This mutex ensures the puck is not modified by the UI thread while being rendered in the rendering thread
    std::mutex bitmapMutex;
};

} // namespace gfx
} // namespace mbgl
