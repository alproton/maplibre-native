
#pragma once

#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <vector>
#include <map>
namespace mbgl {
namespace route {

struct RouteSegmentOptions;
class RouteSegment;

struct RouteOptions {
    Color outerColor = Color(1, 1, 1, 1);
    Color innerColor = Color(0, 0, 1, 1);
    Color outerClipColor = Color(0, 0, 0, 0);
    Color innerClipColor = Color(0, 0, 0, 0);
    float outerWidth = 10;
    float innerWidth = 6;
    std::string layerBefore;
};

/***
 * A route is a polyline that represents a road between two locations. There can be multiple routes in the case of
 * multiple stops. Each route can have route segments. Routes segments represents a traffic zone. A route must have a
 * base route segment (aka casing) that contains all the vertices of the polyline that makes it.
 */
class Route {
public:
    Route() = default;
    Route(const LineString<double>& geometry, const RouteOptions& ropts);
    void routeSegmentCreate(const RouteSegmentOptions&);
    std::map<double, mbgl::Color> getRouteSegmentColorStops(const mbgl::Color& routeColor);
    std::map<double, mbgl::Color> getRouteColorStops(const mbgl::Color& routeColor) const;
    std::vector<double> getRouteSegmentDistances() const;
    void routeSetProgress(const double t);
    double routeGetProgress() const;
    double getTotalDistance() const;
    double getProgressPercent(const Point<double>& progressPoint) const;
    mbgl::LineString<double> getGeometry() const;
    bool hasRouteSegments() const;
    const RouteOptions& getRouteOptions() const;
    bool routeSegmentsClear();
    Route& operator=(Route& other) noexcept;
    uint32_t getNumRouteSegments() const;

private:
    RouteOptions routeOptions_;
    double progress_ = 0.0;
    std::vector<double> segDistances_;
    std::vector<RouteSegment> segments_;
    mbgl::LineString<double> geometry_;
    std::map<double, mbgl::Color> segGradient_;
    double totalDistance_ = 0.0;
};

} // namespace route
} // namespace mbgl
