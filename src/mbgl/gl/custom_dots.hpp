#pragma once

#include <mbgl/gfx/custom_dots.hpp>
#include <mbgl/gl/context.hpp>

namespace mbgl {
namespace gl {

class CustomDots final : public gfx::CustomDots {
public:
    CustomDots(gl::Context&);

    ~CustomDots() noexcept override {}

    void drawImpl() override;
    void updateVertexBufferImpl(const CustomDotsVertexBuffer& vertices) override;
    void clearVertexBufferImpl() override;

private:
    UniqueProgram program;
    VertexArrayID vao = 0;
    VertexArrayID vbo = 0;
    size_t vboSize = 0;
};

} // namespace gl
} // namespace mbgl
