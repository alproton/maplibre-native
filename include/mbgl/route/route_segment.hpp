//
// Created by spalaniappan on 1/6/25.
//

#include <mbgl/route/route_options.hpp>

#pragma once

namespace mbgl {
namespace route {
class RouteSegment {
public:
    RouteSegment() = default;
    RouteSegment(const RouteSegmentOptions& routeOptions);
    void update(const RouteSegmentOptions& routeOptions);
    RouteSegmentOptions getRouteOptions() const;
    ~RouteSegment();

private:
    RouteSegmentOptions options_;

};
} // namespace gfx
} // namespace mbgl
