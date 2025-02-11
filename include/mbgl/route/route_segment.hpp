
#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <mbgl/route/route.hpp>

#pragma once

namespace mbgl {
namespace route {

struct RouteSegmentOptions {
    Color color = Color(1.0f, 1.f, 1.0f, 1.0f);
    LineString<double> geometry;
    uint32_t sortOrder = 0;
};

class RouteSegment {
public:
    RouteSegment() = default;
    RouteSegment(const RouteSegmentOptions& routeOptions);
    RouteSegmentOptions getRouteSegmentOptions() const;
    uint32_t getSortOrder() const;
    ~RouteSegment();

private:
    RouteSegmentOptions options_;
    uint32_t sortOrder_ = 0;

};
} // namespace route
} // namespace mbgl
