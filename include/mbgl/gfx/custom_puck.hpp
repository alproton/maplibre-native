#pragma once

#include <mbgl/map/transform_state.hpp>

namespace mbgl {
namespace gfx {

struct CustomPuckState {
    double lat = 0;
    double lon = 0;
    double bearing = 0;
    bool camera_tracking = false;
    bool visible = false;
};

class CustomPuck {
protected:
    CustomPuck() = default;

public:
    virtual ~CustomPuck() = default;

    virtual void draw(const TransformState& transform) = 0;
};

} // namespace gfx
} // namespace mbgl
