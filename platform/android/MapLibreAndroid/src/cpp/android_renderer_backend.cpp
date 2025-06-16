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

void AndroidRendererBackend::setPuckBitmap(const PremultipliedImage& image) {
    if (!getImpl().customPuck) {
        Log::Debug(Event::Android, "Custom puck not enabled");
        return;
    }
    getImpl().customPuck->setPuckBitmap(image);
}

void AndroidRendererBackend::setCustomDotsNextLayer(std::string layer) {
    if (!isCustomDotsInitialized()) {
        Log::Error(Event::Android, "Custom dots not initialized yet. Ignoring layer");
        return;
    }
    getImpl().customDots->nextLayer(std::move(layer));
}

void AndroidRendererBackend::setCustomDotsPoints(int id, gfx::CustomDotsPoints points) {
    if (!isCustomDotsInitialized()) {
        Log::Error(Event::Android, "Custom dots not initialized yet. Ignoring points");
        return;
    }
    getImpl().customDots->setPoints(id, std::move(points));
}

void AndroidRendererBackend::clearCustomDotsVideoMemory() {
    if (!isCustomDotsInitialized()) {
        Log::Error(Event::Android, "Custom dots not initialized yet. Ignoring clear video memory");
        return;
    }
    getImpl().customDots->deallocateDotsMemory();
}

void AndroidRendererBackend::setCustomDotsOptions(int id, const gfx::CustomDotsOptions& options) {
    if (!isCustomDotsInitialized()) {
        Log::Error(Event::Android, "Custom dots not initialized yet. Ignoring options");
        return;
    }
    getImpl().customDots->setOptions(id, options);
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

bool AndroidRendererBackend::isCustomBlueLineInitialized() {
    return getImpl().customBlueLine != nullptr;
}

void AndroidRendererBackend::clearCustomBlueLine() {
    if (!isCustomBlueLineInitialized()) {
        Log::Error(Event::Android, "Custom blue line not initialized yet. Ignoring blue line");
        return;
    }
    getImpl().customBlueLine->clearCustomBlueLine();
}

void AndroidRendererBackend::setCustomBlueLine(LineString<double> line) {
    if (!isCustomBlueLineInitialized()) {
        Log::Error(Event::Android, "Custom blue line not initialized yet. Ignoring blue line");
        return;
    }
    getImpl().customBlueLine->setCustomBlueLine(std::move(line));
}

void AndroidRendererBackend::setCustomBlueLinePercent(double percent) {
    if (!isCustomBlueLineInitialized()) {
        Log::Error(Event::Android, "Custom blue line not initialized yet. Ignoring blue line");
        return;
    }
    getImpl().customBlueLine->setCustomBlueLinePercent(percent);
}

} // namespace android
} // namespace mbgl
