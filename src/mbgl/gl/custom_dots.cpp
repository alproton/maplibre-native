#include <mbgl/gl/custom_dots.hpp>
#include <mbgl/gl/defines.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/shaders/gl/custom_dots.hpp>
#include <mbgl/util/instrumentation.hpp>
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
        MBGL_CHECK_ERROR(glGetBooleanv(GL_BLEND, &blendEnabled));
        MBGL_CHECK_ERROR(glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled));
        MBGL_CHECK_ERROR(glGetBooleanv(GL_STENCIL_TEST, &stencilTestEnabled));
        MBGL_CHECK_ERROR(glGetBooleanv(GL_CULL_FACE, &cullFaceEnabled));
        MBGL_CHECK_ERROR(glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc));
        MBGL_CHECK_ERROR(glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst));
        MBGL_CHECK_ERROR(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao));
        MBGL_CHECK_ERROR(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo));
        MBGL_CHECK_ERROR(glGetIntegerv(GL_CURRENT_PROGRAM, &program));

        // Set custom puck states
        MBGL_CHECK_ERROR(glBindVertexArray(0));
        MBGL_CHECK_ERROR(glEnable(GL_BLEND));
        MBGL_CHECK_ERROR(glDisable(GL_DEPTH_TEST));
        MBGL_CHECK_ERROR(glDisable(GL_STENCIL_TEST));
        MBGL_CHECK_ERROR(glDisable(GL_CULL_FACE));
        MBGL_CHECK_ERROR(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    }

    ~ScopedGlStates() {
        // Restore states
        enableGlSate(GL_BLEND, blendEnabled);
        enableGlSate(GL_DEPTH_TEST, depthTestEnabled);
        enableGlSate(GL_STENCIL_TEST, stencilTestEnabled);
        enableGlSate(GL_CULL_FACE, cullFaceEnabled);
        MBGL_CHECK_ERROR(glUseProgram(program));
        MBGL_CHECK_ERROR(glBlendFunc(blendSrc, blendDst));
        MBGL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        MBGL_CHECK_ERROR(glBindVertexArray(vao));
    }

private:
    void enableGlSate(GLenum state, GLboolean enabled) {
        if (enabled) {
            MBGL_CHECK_ERROR(glEnable(state));
        } else {
            MBGL_CHECK_ERROR(glDisable(state));
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

CustomDots::CustomDots(gl::Context& context_)
    : context(context_),
      program(createShader(context_)) {}

CustomDots::~CustomDots() noexcept {
    clearVertexBufferImpl();
}

void CustomDots::clearVertexBufferImpl() {
    if (vbo == 0) {
        return;
    }

    context.renderingStats().memVertexBuffers -= vboSize;
    MLN_TRACE_FREE_VERTEX_BUFFER(vbo);

    MBGL_CHECK_ERROR(glDeleteBuffers(1, &vbo));
    MBGL_CHECK_ERROR(glDeleteVertexArrays(1, &vao));
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
        MBGL_CHECK_ERROR(glGenVertexArrays(1, &vao));
        MBGL_CHECK_ERROR(glBindVertexArray(vao));
        MBGL_CHECK_ERROR(glGenBuffers(1, &vbo));
        MBGL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        MBGL_CHECK_ERROR(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
        MBGL_CHECK_ERROR(glVertexAttribDivisor(0, 1));
        MBGL_CHECK_ERROR(glEnableVertexAttribArray(0));
        MBGL_CHECK_ERROR(glDisableVertexAttribArray(1));
        MBGL_CHECK_ERROR(glDisableVertexAttribArray(2));
        MBGL_CHECK_ERROR(glDisableVertexAttribArray(3));
        MBGL_CHECK_ERROR(glDisableVertexAttribArray(4));
    }

    MBGL_CHECK_ERROR(glBindVertexArray(vao));
    MBGL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    if (vboSize < vertices.size()) {
        if (vboSize > 0) {
            context.renderingStats().memVertexBuffers -= vboSize;
            MLN_TRACE_FREE_VERTEX_BUFFER(vbo);
        }
        MBGL_CHECK_ERROR(
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW));
        vboSize = vertices.size();
        context.renderingStats().memVertexBuffers += vboSize;
        MLN_TRACE_ALLOC_VERTEX_BUFFER(vbo, vboSize);
    } else {
        MBGL_CHECK_ERROR(glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data()));
    }
}

void CustomDots::drawImpl() {
    MLN_TRACE_FUNC();

    ScopedGlStates glStates;

    MBGL_CHECK_ERROR(glBindVertexArray(vao));
    MBGL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    MBGL_CHECK_ERROR(glUseProgram(program));

    for (const auto& entry : data) {
        const auto& p = entry.second;
        const auto& inner = p.options.innerColor;
        const auto& outer = p.options.outerColor;
        std::ptrdiff_t bufferOffset = p.vertexOffset * 2 * sizeof(float);
        MBGL_CHECK_ERROR(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(bufferOffset)));
        MBGL_CHECK_ERROR(glUniform2f(1, p.iconDx, p.iconDy));
        MBGL_CHECK_ERROR(glUniform3f(2, inner.r, inner.g, inner.b));
        MBGL_CHECK_ERROR(glUniform3f(3, outer.r, outer.g, outer.b));
        MBGL_CHECK_ERROR(glUniform1f(4, p.innerFactor));
        MBGL_CHECK_ERROR(glDrawArraysInstanced(GL_TRIANGLES, 0, 6, p.vertexCount));
    }
}

} // namespace gl
} // namespace mbgl
