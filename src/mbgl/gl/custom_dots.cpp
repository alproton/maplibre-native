#include <mbgl/gl/custom_dots.hpp>
#include <mbgl/gl/defines.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <cstddef>

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace mbgl {
namespace gl {

using namespace platform;

namespace {

UniqueProgram createShader(gl::Context& context) {
    const char* vs = R"(
#version 310 es
layout (location = 0) in vec2 pos;
layout(location = 1) uniform vec2 size;
out mediump vec2 uv;
void main() {
  mediump float dx = size.x;
  mediump float dy = size.y;
  vec2 trianglePos[6] = vec2[6](vec2(-dx,-dy), vec2(+dx,-dy), vec2(-dx,+dy), vec2(-dx,+dy), vec2(+dx,-dy), vec2(+dx,+dy));
  vec2 triangleUv[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(0, 1), vec2(1, 0), vec2(1, 1));
  gl_Position = vec4(pos + trianglePos[gl_VertexID], 0, 1);
  uv = triangleUv[gl_VertexID];
}
    )";
    const char* ps = R"(
#version 310 es
out mediump vec4 fragColor;
in mediump vec2 uv;
layout(location = 2) uniform mediump vec3 innerColor;
layout(location = 3) uniform mediump vec3 outerColor;
layout(location = 4) uniform mediump float innerFactor;
void main() {
  mediump vec2 p = uv * 2.0 - vec2(1.0);
  mediump float outer = 1.0 - min(pow(dot(p, p), 4.0), 1.0);
  mediump float inner = 1.0 - min(pow(dot(p, p) * innerFactor, 4.0), 1.0);
  mediump vec3 color = mix(outerColor, innerColor, inner);
  fragColor = vec4(color, 1.0) * outer;
}
    )";
    auto vertexShader = context.createShader(gl::ShaderType::Vertex, {vs});
    auto fragmentShader = context.createShader(gl::ShaderType::Fragment, {ps});
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
        glBlendFunc(blendSrc, blendDst);
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
    GLboolean blendEnabled;
    GLboolean depthTestEnabled;
    GLboolean stencilTestEnabled;
    GLboolean cullFaceEnabled;
    GLint blendSrc;
    GLint blendDst;
    GLint vao;
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
