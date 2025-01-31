#include <mbgl/style/layers/route_layer_properties.hpp>

namespace mbgl {
namespace style {

RouteLayerProperties::RouteLayerProperties(
    Immutable<RouteLayer::Impl> impl_)
    : LayerProperties(std::move(impl_)) {}

RouteLayerProperties::RouteLayerProperties(
    Immutable<RouteLayer::Impl> impl_,
    RoutePaintProperties::PossiblyEvaluated evaluated_)
  : LayerProperties(std::move(impl_)),
    evaluated(std::move(evaluated_)) {}

RouteLayerProperties::~RouteLayerProperties() = default;

unsigned long RouteLayerProperties::constantsMask() const {
    return evaluated.constantsMask();
}

const RouteLayer::Impl& RouteLayerProperties::layerImpl() const noexcept {
    return static_cast<const RouteLayer::Impl&>(*baseImpl);
}

expression::Dependency RouteLayerProperties::getDependencies() const noexcept {
    return layerImpl().paint.getDependencies() | layerImpl().layout.getDependencies();
}


}
}