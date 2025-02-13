
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
    std::vector<double> getRouteSegmentDistances() const;
    double getTotalDistance() const;
    mbgl::LineString<double> getGeometry() const;
    bool hasRouteSegments() const;
    //Implement a visitor to visit the routes for gradient expressions
    bool clear();
    bool getGradientDirty() const;
    void validateGradientDirty();
    Route& operator=(Route& other) noexcept;

private:
    bool gradientDirty_ = true;
    void sortRouteSegments();
    std::vector<double> segDistances_;
    std::vector<RouteSegment> segments_;
    mbgl::LineString<double> geometry_;
    double totalDistance_ = 0.0;

};

} // namespace gfx
} // namespace mbgl
