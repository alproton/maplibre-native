//
// Created by spalaniappan on 1/6/25.
//

#include <mbgl/route/route_segment.hpp>

namespace mbgl {
namespace route {

RouteSegment::RouteSegment(const RouteSegmentOptions& routeOptions)
    : options_(routeOptions) {}

RouteSegmentOptions RouteSegment::getRouteSegmentOptions() const {
    return options_;
}

uint32_t RouteSegment::getSortOrder() const {
    return sortOrder_;
}

RouteSegment::~RouteSegment() {}
} // namespace route
} // namespace mbgl
