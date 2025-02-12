
#pragma once

#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <vector>

namespace mbgl {
namespace route {


struct RouteSegmentOptions;
class RouteSegment;

class Route {
    public:
    Route() = default;
    Route(const LineString<double>& geometry);
    void routeSegmentCreate(const RouteSegmentOptions&);
    mbgl::LineString<double> getGeometry() const;
    //Implement a visitor to visit the routes for gradient expressions
    bool clear();
    bool getDirty() const;
    Route& operator=(Route& other) noexcept;

private:
    bool dirty_ = true;
    void sortRouteSegments();
    //Keep an ordered list of routesegments
    std::vector<RouteSegment> segments_;
    mbgl::LineString<double> geometry_;
    // uint32_t sortOrder_ = 0;

};

} // namespace gfx
} // namespace mbgl
