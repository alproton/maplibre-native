
#pragma once

#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <vector>
#include <map>
namespace mbgl {
namespace route {


struct RouteSegmentOptions;
class RouteSegment;

/**
 * A
 **/
class Route {
    public:
    Route() = default;
    Route(const LineString<double>& geometry);
    void routeSegmentCreate(const RouteSegmentOptions&);
    std::map<double, mbgl::Color> getRouteSegmentColorStops(const mbgl::Color& routeColor) const;
    std::map<double, mbgl::Color> getRouteColorStops(const mbgl::Color& routeColor) const;
    std::vector<double> getRouteSegmentDistances() const;
    bool routeSetProgress(const double t);
    double getTotalDistance() const;
    mbgl::LineString<double> getGeometry() const;
    bool hasRouteSegments() const;
    //Implement a visitor to visit the routes for gradient expressions
    bool routeSegmentsClear();
    bool getGradientDirty() const;
    void validateGradientDirty();
    Route& operator=(Route& other) noexcept;

private:
    static const double EPSILON ;
    bool gradientDirty_ = true;
    double progress_ = 0.0;
    void sortRouteSegments();
    std::vector<double> segDistances_;
    std::vector<RouteSegment> segments_;
    mbgl::LineString<double> geometry_;
    double totalDistance_ = 0.0;
    mbgl::Color progressColor_ = Color(0.0, 0.0, 0.0, 0.0);
};

} // namespace gfx
} // namespace mbgl
