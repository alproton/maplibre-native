
#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <mbgl/route/route.hpp>
#include <string>

#pragma once

namespace mbgl {
namespace route {

struct RouteSegmentOptions {
    static const std::string BASE_ROUTE_SEGMENT_STR ;

    Color color = Color(1.0f, 1.f, 1.0f, 1.0f);
    LineString<double> geometry;
    uint32_t sortOrder = 0;
    std::string name;
};

class RouteSegment {
public:
    RouteSegment() = default;
    RouteSegment(const RouteSegmentOptions& routeOptions);
    RouteSegmentOptions getRouteSegmentOptions() const;
    ~RouteSegment();

private:
    RouteSegmentOptions options_;

};
} // namespace gfx
} // namespace mbgl
