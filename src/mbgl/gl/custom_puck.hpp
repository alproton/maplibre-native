#pragma once

#include <mbgl/gfx/custom_puck.hpp>
#include <mbgl/gl/context.hpp>

namespace mbgl {
namespace gl {

class CustomPuck final : public gfx::CustomPuck {
public:
    CustomPuck(gl::Context&);

    ~CustomPuck() noexcept override {}

    void drawImpl(const ScreenQuad& quad) override;
    gfx::CustomPuckState getState() override;

    void drawChargerImpl(const std::vector<float>& vertices, float dx, float dy) override;

private:
    gl::Context& context;
    UniqueProgram program;
    TextureID texture = 0;

    VertexArrayID vao = 0;
    VertexArrayID vbo = 0;
    UniqueProgram chargerProgram;
};

} // namespace gl
} // namespace mbgl
