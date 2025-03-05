#include <mbgl/gl/custom_puck.hpp>
#include <mbgl/gl/defines.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/shaders/gl/custom_puck.hpp>
#include <mbgl/util/logging.hpp>
#include <algorithm>

// This is a temporary workaround to issue #3135
// We use glGetFloatv to get the max anisotropy instead of checking the existence of the extension
// This extension is virtually supported by all GPUs since it's a D3D9 feature
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace mbgl {
namespace gl {

using namespace platform;

namespace {

UniqueProgram createPuckShader(gl::Context& context) {
    using shader = shaders::ShaderSource<shaders::BuiltIn::CustomPuckProgram, gfx::Backend::Type::OpenGL>;
    auto vertexShader = context.createShader(gl::ShaderType::Vertex, {shader::vertex});
    auto fragmentShader = context.createShader(gl::ShaderType::Fragment, {shader::fragment});
    auto program = context.createProgram(vertexShader, fragmentShader, nullptr);
    return program;
}

TextureID createTexture(const PremultipliedImage& image) {
    GLuint texture;
    MBGL_CHECK_ERROR(glGenTextures(1, &texture));
    MBGL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
    MBGL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D,
                                  0,
                                  GL_RGBA,
                                  image.size.width,
                                  image.size.height,
                                  0,
                                  GL_RGBA,
                                  GL_UNSIGNED_BYTE,
                                  image.data.get()));
    // Generate mipmaps
    MBGL_CHECK_ERROR(glGenerateMipmap(GL_TEXTURE_2D));
    // Set sampler
    MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    // Enable aniso filtering
    GLfloat maxAniso = 0.0f;
    MBGL_CHECK_ERROR(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso));
    if (maxAniso > 0) {
        constexpr float aniso = 8.0f;
        MBGL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(maxAniso, aniso)));
    } else {
        Log::Warning(Event::OpenGL,
                     "Anisotropic filtering not supported: Puck texture will be renderer with tri-linear filtering.");
    }
    return texture;
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
        MBGL_CHECK_ERROR(glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBinding));
        MBGL_CHECK_ERROR(glGetIntegerv(GL_ACTIVE_TEXTURE, &textureUnit));
        MBGL_CHECK_ERROR(glGetIntegerv(GL_CURRENT_PROGRAM, &program));
        // Set custom puck states
        MBGL_CHECK_ERROR(glBindVertexArray(0));
        MBGL_CHECK_ERROR(glEnable(GL_BLEND));
        MBGL_CHECK_ERROR(glDisable(GL_DEPTH_TEST));
        MBGL_CHECK_ERROR(glDisable(GL_STENCIL_TEST));
        MBGL_CHECK_ERROR(glDisable(GL_CULL_FACE));
        MBGL_CHECK_ERROR(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
        MBGL_CHECK_ERROR(glActiveTexture(GL_TEXTURE0));
    }

    ~ScopedGlStates() {
        // Restore states
        enableGlSate(GL_BLEND, blendEnabled);
        enableGlSate(GL_DEPTH_TEST, depthTestEnabled);
        enableGlSate(GL_STENCIL_TEST, stencilTestEnabled);
        enableGlSate(GL_CULL_FACE, cullFaceEnabled);
        MBGL_CHECK_ERROR(glUseProgram(program));
        MBGL_CHECK_ERROR(glBlendFunc(blendSrc, blendDst));
        MBGL_CHECK_ERROR(glBindVertexArray(vao));
        MBGL_CHECK_ERROR(glActiveTexture(textureUnit));
        MBGL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, textureBinding));
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
    GLint textureBinding = 0;
    GLint textureUnit = 0;
};

} // namespace

CustomPuck::CustomPuck(gl::Context& context_)
    : context(context_),
      program(createPuckShader(context_)) {}

void CustomPuck::drawImpl(const ScreenQuad& quad) {
    ScopedGlStates glStates;

    auto bitmap = getPuckBitmap();
    if (bitmap.valid()) {
        // Create or recreate the texture because the bitmap has changed
        if (texture) {
            MBGL_CHECK_ERROR(glDeleteTextures(1, &texture));
            context.renderingStats().memTextures -= storage;
            MLN_TRACE_ALLOC_TEXTURE(texture, storage);
        }
        texture = createTexture(bitmap);
        storage = bitmap.size.width * bitmap.size.height * 4;
        context.renderingStats().memTextures += storage;
        MLN_TRACE_ALLOC_TEXTURE(texture, storage);
    }
    if (!texture) {
        // The UI thread has not set the puck bitmap yet
        // Skip puck rendering in this frame while MapView is being initialized
        return;
    }

    MBGL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
    MBGL_CHECK_ERROR(glUseProgram(program));
    for (int i = 0; i < 4; ++i) {
        MBGL_CHECK_ERROR(glUniform2f(i, static_cast<float>(quad[i].x), static_cast<float>(quad[i].y)));
    }
    MBGL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
}

gfx::CustomPuckState CustomPuck::getState() {
    return context.getBackend().getCurrentCustomPuckState();
}

CustomPuck::~CustomPuck() noexcept {
    // Only the texture is deleted here when the program ends
    // The shader is deleted by the context
    if (texture) {
        MBGL_CHECK_ERROR(glDeleteTextures(1, &texture));
        context.renderingStats().memTextures -= storage;
        MLN_TRACE_ALLOC_TEXTURE(texture, storage);
    }
}

} // namespace gl
} // namespace mbgl
