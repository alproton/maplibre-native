#include <mbgl/gl/custom_puck.hpp>
#include <mbgl/gl/defines.hpp>
#include <mbgl/gl/renderer_backend.hpp>

namespace mbgl {
namespace gl {

using namespace platform;

namespace {

UniqueProgram createPuckShader(gl::Context& context) {
    const char* vs = R"(
#version 310 es
layout(location = 0) uniform vec2 v0;
layout(location = 1) uniform vec2 v1;
layout(location = 2) uniform vec2 v2;
layout(location = 3) uniform vec2 v3;
out mediump vec2 uv;
void main() {
  vec2 pos[4] = vec2[4](v0, v1, v2, v3);
  vec2 vertex_uv[4] = vec2[4](vec2(0,0), vec2(1,0), vec2(0,1), vec2(1,1));
  gl_Position = vec4(pos[gl_VertexID], 0, 1);
  uv = vertex_uv[gl_VertexID];
}
    )";
    const char* ps = R"(
#version 310 es
in mediump vec2 uv;
out mediump vec4 fragColor;
void main() {
  mediump float c = (1.f - abs(uv.x * 2.f - 1.f)) * (1.f - uv.y);
  c = clamp(pow(c * 2.f, 2.f), 0.f, 1.f);
  fragColor = vec4(0, 1, 0, c);
}
    )";
    auto vertexShader = context.createShader(gl::ShaderType::Vertex, {vs});
    auto fragmentShader = context.createShader(gl::ShaderType::Fragment, {ps});
    auto program = context.createProgram(vertexShader, fragmentShader, nullptr);
    return program;
}

class ScopedGlStates {
public:
    ScopedGlStates() {
        glGetBooleanv(GL_BLEND, &blendEnabled);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);

        glBindVertexArray(0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ~ScopedGlStates() {
        if (blendEnabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        glBlendFunc(blendSrc, blendDst);
        glBindVertexArray(vao);
    }

private:
    GLboolean blendEnabled;
    GLint blendSrc;
    GLint blendDst;
    GLint vao;
};

} // namespace

CustomPuck::CustomPuck(gl::Context& context_)
    : context(context_),
      program(createPuckShader(context_)) {}

void CustomPuck::drawImpl(const ScreenQuad& quad) {
    ScopedGlStates glStates;

    glUseProgram(program);
    for (int i = 0; i < 4; ++i) {
        glUniform2f(i, static_cast<float>(quad[i].x), static_cast<float>(quad[i].y));
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

gfx::CustomPuckState CustomPuck::getState() {
    return context.getBackend().getCurrentCustomPuckState();
}

} // namespace gl
} // namespace mbgl
