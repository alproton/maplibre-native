#pragma once

#include <mbgl/style/types.hpp>
#include <mbgl/style/layer_properties.hpp>
#include <mbgl/style/layers/line_layer.hpp>
#include <mbgl/style/layout_property.hpp>
#include <mbgl/style/paint_property.hpp>
#include <mbgl/style/properties.hpp>
#include <mbgl/programs/attributes.hpp>
#include <mbgl/programs/uniforms.hpp>
#include <mbgl/style/layers/line_layer_properties.hpp>
#include <mbgl/style/layers/route_layer_impl.hpp>

namespace mbgl {
namespace style {

class RoutePaintProperties : public Properties<
    LineBlur,
    LineColor,
    LineDasharray,
    LineFloorWidth,
    LineGapWidth,
    LineGradient,
    LineOffset,
    LineOpacity,
    LinePattern,
    LineTranslate,
    LineTranslateAnchor,
    LineWidth,
    LineVanishDistance> {};

class RouteLayerProperties final : public LayerProperties {
public:
    explicit RouteLayerProperties(Immutable<RouteLayer::Impl>);
    RouteLayerProperties(
        Immutable<RouteLayer::Impl>,
        RoutePaintProperties::PossiblyEvaluated);
    ~RouteLayerProperties() override;

    unsigned long constantsMask() const override;

    expression::Dependency getDependencies() const noexcept override;

    const RouteLayer::Impl& layerImpl() const noexcept;
    // Data members.
    RoutePaintProperties::PossiblyEvaluated evaluated;

};


}
}