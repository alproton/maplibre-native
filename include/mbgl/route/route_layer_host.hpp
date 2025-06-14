#pragma once

#include <mbgl/style/layers/custom_drawable_layer.hpp>
#include <mbgl/route/route.hpp>

namespace mbgl {
namespace route {

class RouteLayerHost : public mbgl::style::CustomDrawableLayerHost {
public:
    using VertexVector = mbgl::gfx::VertexVector<Interface::GeometryVertex>;
    using TriangleIndexVector = mbgl::gfx::IndexVector<mbgl::gfx::Triangles>;

    RouteLayerHost();
    void addRoute(const LineString<double>& routegeometry, const RouteOptions& ropts);

    void initialize() override;
    void deinitialize() override;

    void update(Interface& interface) override;

protected:
    static mbgl::Point<double> project(const mbgl::LatLng& c, const mbgl::TransformState& s);
    void createDrawables(Interface& interface);

private:
    struct RouteData {
        LineString<double> routeGeom;
        RouteOptions routeOptions;
    };

    std::vector<RouteData> routeDataList_;
};

} // namespace route
} // namespace mbgl
