#pragma once

#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/image.hpp>
#include <array>
#include <memory>

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
    virtual ~CustomPuck() = default;

    void draw(const TransformState& transform);

protected:
    using ScreenQuad = std::array<ScreenCoordinate, 4>;

    virtual void drawImpl(const ScreenQuad&) = 0;

    virtual CustomPuckState getState() = 0;
};

void setPuckBitmap(const PremultipliedImage& src);
const PremultipliedImage& getPuckBitmap();

} // namespace gfx
} // namespace mbgl
