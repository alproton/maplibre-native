#pragma once

#include "puck_view.hpp"
#include <mbgl/renderer/renderer_frontend.hpp>
#include <mbgl/util/feature.hpp>
#include <memory>

namespace mbgl {
class Renderer;
} // namespace mbgl

class PUCKRendererFrontend : public mbgl::RendererFrontend {
public:
    PUCKRendererFrontend(std::unique_ptr<mbgl::Renderer>, PUCKView&);
    ~PUCKRendererFrontend() override;

    void reset() override;
    void setObserver(mbgl::RendererObserver&) override;

    void update(std::shared_ptr<mbgl::UpdateParameters>) override;
    const mbgl::TaggedScheduler& getThreadPool() const override;
    void render();

    std::vector<mbgl::Feature> queryFeatures(double screenspaceX, double screenspaceY);

    mbgl::Renderer* getRenderer();

private:
    PUCKView& glfwView;
    std::unique_ptr<mbgl::Renderer> renderer;
    std::shared_ptr<mbgl::UpdateParameters> updateParameters;
};
