#include "android_gl_renderer_backend.hpp"

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gl/context.hpp>
#include <mbgl/gl/renderable_resource.hpp>
#include <mbgl/util/logging.hpp>
#include "android_egl_helper.hpp"

#include <cassert>

namespace mbgl {
namespace android {

class AndroidGLRenderableResource final : public mbgl::gl::RenderableResource {
public:
    AndroidGLRenderableResource(AndroidGLRendererBackend& backend_)
        : backend(backend_) {}

    void bind() override {
        assert(gfx::BackendScope::exists());
        backend.setFramebufferBinding(0);
        backend.setViewport(0, 0, backend.getSize());
    }

    void swap() override {
        const auto& swapBehaviour = static_cast<AndroidGLRendererBackend&>(backend).getSwapBehavior();
        if (swapBehaviour == gfx::Renderable::SwapBehaviour::Flush) {
            static_cast<gl::Context&>(backend.getContext()).finish();
        }
    }

private:
    AndroidGLRendererBackend& backend;
};

AndroidGLRendererBackend::AndroidGLRendererBackend()
    : gl::RendererBackend(gfx::ContextMode::Unique),
      mbgl::gfx::Renderable({64, 64}, std::make_unique<AndroidGLRenderableResource>(*this)) {}

AndroidGLRendererBackend::~AndroidGLRendererBackend() = default;

gl::ProcAddress AndroidGLRendererBackend::getExtensionFunctionPointer(const char* name) {
    assert(gfx::BackendScope::exists());
    return eglGetProcAddress(name);
}

void AndroidGLRendererBackend::updateViewPort() {
    assert(gfx::BackendScope::exists());
    setViewport(0, 0, size);
    if (logEGLconfigAttribs) {
        Log::Info(Event::OpenGL, "EGL Config Attributes:\n" + android::egl::query_current_config_attributes());
        logEGLconfigAttribs = false;
    }
}

void AndroidGLRendererBackend::resizeFramebuffer(int width, int height) {
    size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

PremultipliedImage AndroidGLRendererBackend::readFramebuffer() {
    assert(gfx::BackendScope::exists());
    return gl::RendererBackend::readFramebuffer(size);
}

void AndroidGLRendererBackend::updateAssumedState() {
    assumeFramebufferBinding(0);
    assumeViewport(0, 0, size);
}

void AndroidGLRendererBackend::markContextLost() {
    if (context) {
        getContext<gl::Context>().setCleanupOnDestruction(false);
    }
}

void AndroidGLRendererBackend::setSwapInterval(int interval) {
    if (swapInterval != interval) {
        swapInterval = interval;
        bool success = eglSwapInterval(eglGetCurrentDisplay(), swapInterval);
        if (!success) {
            Log::Info(Event::OpenGL, "Failure in setting swap interval");
        } else {
            Log::Info(Event::OpenGL, "Setting swap interval to " + std::to_string(swapInterval));
        }
    }
}

} // namespace android
} // namespace mbgl

namespace mbgl {
namespace gfx {

template <>
std::unique_ptr<android::AndroidRendererBackend> Backend::Create<mbgl::gfx::Backend::Type::OpenGL>(ANativeWindow*) {
    return std::make_unique<android::AndroidGLRendererBackend>();
}

} // namespace gfx
} // namespace mbgl
