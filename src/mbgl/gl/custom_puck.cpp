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
// Since this is a temporary workaround to issue #3135, we use
// uniform vectors for the puck quad to simplify the GL side code
layout(location = 0) uniform vec2 v0;
layout(location = 1) uniform vec2 v1;
layout(location = 2) uniform vec2 v2;
layout(location = 3) uniform vec2 v3;
out mediump vec2 uv;
void main() {
  vec2 pos[4] = vec2[4](v0, v1, v2, v3);
  vec2 vertex_uv[4] = vec2[4](vec2(0,1), vec2(1,1), vec2(0,0), vec2(1,0));
  gl_Position = vec4(pos[gl_VertexID], 0, 1);
  uv = vertex_uv[gl_VertexID];
}
    )";
    const char* ps = R"(
#version 310 es
in mediump vec2 uv;
out mediump vec4 fragColor;
uniform sampler2D tex;
void main() {
  fragColor = texture(tex, uv);
}
    )";
    auto vertexShader = context.createShader(gl::ShaderType::Vertex, {vs});
    auto fragmentShader = context.createShader(gl::ShaderType::Fragment, {ps});
    auto program = context.createProgram(vertexShader, fragmentShader, nullptr);
    return program;
}

TextureID createTexture(const PremultipliedImage& image) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, image.size.width, image.size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data.get());
    return texture;
}

class ScopedGlStates final {
public:
    ScopedGlStates() {
        glGetBooleanv(GL_BLEND, &blendEnabled);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBinding);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &textureUnit);

        glBindVertexArray(0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(0);
    }

    ~ScopedGlStates() {
        if (blendEnabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        glBlendFunc(blendSrc, blendDst);
        glBindVertexArray(vao);
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, textureBinding);
    }

private:
    GLboolean blendEnabled;
    GLint blendSrc;
    GLint blendDst;
    GLint vao;
    GLint textureBinding;
    GLint textureUnit;
};

} // namespace

CustomPuck::CustomPuck(gl::Context& context_)
    : context(context_),
      program(createPuckShader(context_)) {}

void CustomPuck::drawImpl(const ScreenQuad& quad) {
    ScopedGlStates glStates;

    if (!texture) {
        const auto& bitmap = gfx::getPuckBitmap();
        if (!bitmap.valid()) {
            return;
        }
        texture = createTexture(bitmap);
    }

    glBindTexture(GL_TEXTURE_2D, texture);
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
