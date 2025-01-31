#pragma once

#include <mbgl/renderer/render_layer.hpp>
#include <mbgl/style/layers/route_layer_impl.hpp>
#include <optional>
#include <memory>

#if MLN_DRAWABLE_RENDERER
namespace mbgl {
class RouteBucket;
using RouteBucketPtr = std::shared_ptr<RouteBucket>;

namespace gfx {
class ShaderGroup;
class UniformBuffer;
using ShaderGroupPtr = std::shared_ptr<ShaderGroup>;
using UniformBufferPtr = std::shared_ptr<UniformBuffer>;
} // namespace gfx
#endif

class RenderRouteLayer : public RenderLayer {
    public:
    explicit RenderRouteLayer(Immutable<style::RouteLayer::Impl>);
    ~RenderRouteLayer() override;

#if MLN_DRAWABLE_RENDERER
    /// Generate any changes needed by the layer
    void update(gfx::ShaderRegistry&,
                gfx::Context&,
                const TransformState&,
                const std::shared_ptr<UpdateParameters>&,
                const RenderTree&,
                UniqueChangeRequestVec&) override;

private:
    void transition(const TransitionParameters&) override;
    void evaluate(const PropertyEvaluationParameters&) override;
    bool hasTransition() const override;
    bool hasCrossfade() const override;
    void prepare(const LayerPrepareParameters&) override;

    // Paint properties
    style::LinePaintProperties::Unevaluated unevaluated;
#if MLN_DRAWABLE_RENDERER
    gfx::ShaderGroupPtr routeShaderGroup;
    RouteBucketPtr routeBucket;
#endif

#endif

};

}