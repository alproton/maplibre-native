//
// Created by spalaniappan on 1/6/25.
//

#include <mbgl/route/RouteSegment.hpp>

namespace mbgl {
namespace gfx {
RouteSegment::RouteSegment(const RouteSegmentOptions& routeOptions, const RouteSegmentHint& rsh)
    : options_(routeOptions), segmentHint_(rsh) {}

void RouteSegment::update(const RouteSegmentOptions& routeOptions) {
    options_ = routeOptions;
}
RouteSegmentOptions RouteSegment::getRouteOptions() const {
    return options_;
}

RouteSegmentHint RouteSegment::getRouteSegmentHint() const {
    return segmentHint_;
}

RouteSegment::~RouteSegment() {}
} // namespace gfx
} // namespace mbgl
