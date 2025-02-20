#pragma once

#include <mbgl/gfx/custom_puck.hpp>
#include <mbgl/gl/context.hpp>

namespace mbgl {
namespace gl {

class CustomPuck : public gfx::CustomPuck {
public:
    CustomPuck(gl::Context&);

    void drawImpl(const ScreenQuad& quad) override;
    gfx::CustomPuckState getState() override;

private:
    gl::Context& context;
    UniqueProgram program;
};

} // namespace gl
} // namespace mbgl
