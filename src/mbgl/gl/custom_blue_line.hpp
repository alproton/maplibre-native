#pragma once

#include <mbgl/gfx/custom_blue_line.hpp>
#include <mbgl/gl/context.hpp>

namespace mbgl {
namespace gl {

class CustomBlueLine final : public gfx::CustomBlueLine {
public:
    CustomBlueLine(gl::Context&);

    ~CustomBlueLine() noexcept override;

    void drawImpl() override;
    void updateVertexBufferImpl(const std::vector<float>& vertices) override;
    void clearVertexBufferImpl() override;

private:
    gl::Context& context;
    UniqueProgram program;
    VertexArrayID vao = 0;
    VertexArrayID vbo = 0;
    size_t vboSize = 0;
    int vertexCount = 0;
};

} // namespace gl
} // namespace mbgl
