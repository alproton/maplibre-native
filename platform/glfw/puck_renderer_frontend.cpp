#include "puck_renderer_frontend.hpp"

#include <mbgl/renderer/renderer.hpp>
#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderer_backend.hpp>
#include <mbgl/util/instrumentation.hpp>
#include <mbgl/renderer/query.hpp>

PUCKRendererFrontend::PUCKRendererFrontend(std::unique_ptr<mbgl::Renderer> renderer_, PUCKView& glfwView_)
    : glfwView(glfwView_),
      renderer(std::move(renderer_)) {
    glfwView.setRenderFrontend(this);
}

PUCKRendererFrontend::~PUCKRendererFrontend() = default;

void PUCKRendererFrontend::reset() {
    assert(renderer);
    renderer.reset();
}

std::vector<mbgl::Feature> PUCKRendererFrontend::queryFeatures(double screenSpaceX, double screenSpaceY) {
    mbgl::ScreenCoordinate screen_point(screenSpaceX, screenSpaceY);
    std::vector<mbgl::Feature> features = renderer->queryRenderedFeatures(screen_point);
    return features;
}

void PUCKRendererFrontend::setObserver(mbgl::RendererObserver& observer) {
    assert(renderer);
    renderer->setObserver(&observer);
}

void PUCKRendererFrontend::update(std::shared_ptr<mbgl::UpdateParameters> params) {
    updateParameters = std::move(params);
    glfwView.invalidate();
}

const mbgl::TaggedScheduler& PUCKRendererFrontend::getThreadPool() const {
    return glfwView.getRendererBackend().getThreadPool();
}

void PUCKRendererFrontend::render() {
    MLN_TRACE_FUNC();

    assert(renderer);

    if (!updateParameters) return;

    mbgl::gfx::BackendScope guard{glfwView.getRendererBackend(), mbgl::gfx::BackendScope::ScopeType::Implicit};

    // onStyleImageMissing might be called during a render. The user implemented
    // method could trigger a call to MLNRenderFrontend#update which overwrites
    // `updateParameters`. Copy the shared pointer here so that the parameters
    // aren't destroyed while `render(...)` is still using them.
    auto updateParameters_ = updateParameters;
    renderer->render(updateParameters_);
}

mbgl::Renderer* PUCKRendererFrontend::getRenderer() {
    assert(renderer);
    return renderer.get();
}
