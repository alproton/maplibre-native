#pragma once

#include <mbgl/map/transform_state.hpp>
#include <array>

namespace mbgl {
namespace gfx {

class Context;

struct CustomPuckState {
    double lat = 0;
    double lon = 0;
    double bearing = 0;
    float iconScale = 1.f;
    bool cameraTracking = false;
};

class CustomPuck {
protected:
    CustomPuck() = default;

public:
    virtual ~CustomPuck() = default;

    void draw(const TransformState& transform);

protected:
    using ScreenQuad = std::array<ScreenCoordinate, 4>;

    virtual void drawImpl(const ScreenQuad&) {}

    virtual CustomPuckState getState() { return {}; }
};

} // namespace gfx
} // namespace mbgl
