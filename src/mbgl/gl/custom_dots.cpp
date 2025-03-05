#include <mbgl/gl/custom_dots.hpp>
#include <mbgl/gl/defines.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/shaders/gl/custom_dots.hpp>
#include <cstddef>

namespace mbgl {
namespace gl {

using namespace platform;

namespace {

UniqueProgram createShader(gl::Context& context) {
    using shader = shaders::ShaderSource<shaders::BuiltIn::CustomDotsProgram, gfx::Backend::Type::OpenGL>;
    auto vertexShader = context.createShader(gl::ShaderType::Vertex, {shader::vertex});
    auto fragmentShader = context.createShader(gl::ShaderType::Fragment, {shader::fragment});
    auto program = context.createProgram(vertexShader, fragmentShader, "pos");
    return program;
}

class ScopedGlStates final {
public:
    ScopedGlStates() {
        // Save states
        glGetBooleanv(GL_BLEND, &blendEnabled);
        glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
        glGetBooleanv(GL_STENCIL_TEST, &stencilTestEnabled);
        glGetBooleanv(GL_CULL_FACE, &cullFaceEnabled);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);

        // Set custom puck states
        glBindVertexArray(0);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    ~ScopedGlStates() {
        // Restore states
        enableGlSate(GL_BLEND, blendEnabled);
        enableGlSate(GL_DEPTH_TEST, depthTestEnabled);
        enableGlSate(GL_STENCIL_TEST, stencilTestEnabled);
        enableGlSate(GL_CULL_FACE, cullFaceEnabled);
        glUseProgram(program);
        glBlendFunc(blendSrc, blendDst);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindVertexArray(vao);
    }

private:
    void enableGlSate(GLenum state, GLboolean enabled) {
        if (enabled) {
            glEnable(state);
        } else {
            glDisable(state);
        }
    }

private:
    GLboolean blendEnabled = false;
    GLboolean depthTestEnabled = false;
    GLboolean stencilTestEnabled = false;
    GLboolean cullFaceEnabled = false;
    GLint program = 0;
    GLint blendSrc = 0;
    GLint blendDst = 0;
    GLint vao = 0;
    GLint vbo = 0;
};

} // namespace

CustomDots::CustomDots(gl::Context& context)
    : program(createShader(context)) {}

void CustomDots::clearVertexBufferImpl() {
    if (vbo == 0) {
        return;
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    vao = 0;
    vbo = 0;
    vboSize = 0;
}

void CustomDots::updateVertexBufferImpl(const CustomDotsVertexBuffer& vertices) {
    if (vertices.empty()) {
        return;
    }

    ScopedGlStates glStates;

    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glVertexAttribDivisor(0, 1);
        glEnableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(4);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (vboSize < vertices.size()) {
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        vboSize = vertices.size();
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    }
}

void CustomDots::drawImpl() {
    ScopedGlStates glStates;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glUseProgram(program);

    for (const auto& entry : data) {
        const auto& p = entry.second;
        const auto& inner = p.options.innerColor;
        const auto& outer = p.options.outerColor;
        std::ptrdiff_t bufferOffset = p.vertexOffset * 2 * sizeof(float);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(bufferOffset));
        glUniform2f(1, p.iconDx, p.iconDy);
        glUniform3f(2, inner.r, inner.g, inner.b);
        glUniform3f(3, outer.r, outer.g, outer.b);
        glUniform1f(4, p.innerFactor);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, p.vertexCount);
    }
}

} // namespace gl
} // namespace mbgl
