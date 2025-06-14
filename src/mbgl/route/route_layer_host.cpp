#include <mbgl/route/route_layer_host.hpp>

namespace mbgl {
namespace route {

RouteLayerHost::RouteLayerHost() {}

void RouteLayerHost::initialize() {}

void RouteLayerHost::deinitialize() {}

void RouteLayerHost::addRoute(const LineString<double>& routegeometry, const RouteOptions& ropts) {
    routeDataList_.push_back({routegeometry, ropts});
}

void RouteLayerHost::update(Interface& interface) {
    // if we have built our drawable(s) already, either update or skip
    if (interface.getDrawableCount() == 0) {
        createDrawables(interface);
        return;
    }
}

mbgl::Point<double> RouteLayerHost::project(const mbgl::LatLng& c, const mbgl::TransformState& s) {
    mbgl::LatLng unwrappedLatLng = c.wrapped();
    unwrappedLatLng.unwrapForShortestPath(s.getLatLng(mbgl::LatLng::Wrapped));
    return mbgl::Projection::project(unwrappedLatLng, s.getScale());
}

void RouteLayerHost::createDrawables(Interface& interface) {
    using namespace mbgl;

    for (const auto& routeData : routeDataList_) {
        const RouteOptions& ropts = routeData.routeOptions;
        const LineString<double> routeGeom = routeData.routeGeom;

        Interface::LineOptions options = {/*geometry=*/{},
                                          /*blur=*/0.0f,
                                          /*opacity=*/1.0f,
                                          /*gapWidth=*/0.0f,
                                          /*offset=*/0.0f,
                                          /*width=*/ropts.innerWidth,
                                          /*color=*/ropts.innerColor};

        options.geometry.beginCap = style::LineCapType::Square;
        options.geometry.endCap = style::LineCapType::Square;
        options.geometry.joinType = style::LineJoinType::Bevel;
        options.geometry.type = FeatureType::LineString;
        interface.setLineOptions(options);

        interface.addPolyline(routeGeom, Interface::LineShaderType::Classic);
    }
}

} // namespace route
} // namespace mbgl
