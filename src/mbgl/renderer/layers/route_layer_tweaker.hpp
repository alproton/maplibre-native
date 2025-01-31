#pragma once

#include <mbgl/renderer/layer_tweaker.hpp>
#include <mbgl/style/layers/route_layer_properties.hpp>

namespace mbgl {
    class RouteLayerTweaker : public LayerTweaker {
    public:
        RouteLayerTweaker(std::string id_, Immutable<style::LayerProperties> properties)
            : LayerTweaker(std::move(id_), properties) {}

        ~RouteLayerTweaker() override = default;

        void execute(LayerGroupBase&, const PaintParameters&) override;
        void setRouteDistanceTraversed(float t);

    private:
        template <typename Property>
        auto evaluate(const PaintParameters& parameters) const;

    protected:
        gfx::UniformBufferPtr evaluatedPropsUniformBuffer;
        float routeDistanceTraversed = 0.33f;
    };
}