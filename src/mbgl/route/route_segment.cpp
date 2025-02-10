//
// Created by spalaniappan on 1/6/25.
//

#include <mbgl/route/route_segment.hpp>

namespace mbgl {
namespace route {

RouteSegment::RouteSegment(const RouteSegmentOptions& routeOptions)
    : options_(routeOptions) {}

void RouteSegment::update(const RouteSegmentOptions& routeOptions) {
    options_ = routeOptions;
}
RouteSegmentOptions RouteSegment::getRouteOptions() const {
    return options_;
}

RouteSegment::~RouteSegment() {}
} // namespace gfx
} // namespace mbgl
