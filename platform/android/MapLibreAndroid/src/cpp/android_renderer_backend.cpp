#include "android_renderer_backend.hpp"
#include <mbgl/util/logging.hpp>
#include <cassert>

namespace mbgl {
namespace android {

void AndroidRendererBackend::updateViewPort() {}

void AndroidRendererBackend::resizeFramebuffer(int width, int height) {}

PremultipliedImage AndroidRendererBackend::readFramebuffer() {
    return PremultipliedImage();
}

void AndroidRendererBackend::markContextLost() {}

void AndroidRendererBackend::setSwapBehavior(gfx::Renderable::SwapBehaviour swapBehaviour_) {
    swapBehaviour = swapBehaviour_;
}

void AndroidRendererBackend::setCustomDotsPoints(gfx::CustomDotsPoints points) {
    if (!isCustomDotsInitialized()) {
        Log::Error(Event::Android, "Custom dots not initialized yet. Ignoring points");
        return;
    }
    getImpl().customDots->setPoints(std::move(points));
}

void AndroidRendererBackend::setCustomDotsOptions(const gfx::CustomDotsOptions& options) {
    if (!isCustomDotsInitialized()) {
        Log::Error(Event::Android, "Custom dots not initialized yet. Ignoring options");
        return;
    }
    getImpl().customDots->setOptions(options);
}

void AndroidRendererBackend::setCustomDotsEnabled(bool enabled) {
    if (!isCustomDotsInitialized()) {
        Log::Error(Event::Android, "Custom dots not initialized yet and connot be enabled");
        return;
    }
    getImpl().customDots->setEnabled(enabled);
}

bool AndroidRendererBackend::isCustomDotsInitialized() {
    return getImpl().customDots != nullptr;
}

} // namespace android
} // namespace mbgl
