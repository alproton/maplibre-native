//
// Created by spalaniappan on 1/6/25.
//

#include <mbgl/route/route_segment.hpp>

namespace mbgl {
namespace route {

const std::string RouteSegmentOptions::BASE_ROUTE_SEGMENT_STR = "base_route_segment";

RouteSegment::RouteSegment(const RouteSegmentOptions& routeOptions)
    : options_(routeOptions) {}

RouteSegmentOptions RouteSegment::getRouteSegmentOptions() const {
    return options_;
}

RouteSegment::~RouteSegment() {}
} // namespace gfx
} // namespace mbgl
