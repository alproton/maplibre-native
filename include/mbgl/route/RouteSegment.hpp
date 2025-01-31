//
// Created by spalaniappan on 1/6/25.
//

#include <mbgl/route/route_options.hpp>

#pragma once

namespace mbgl {
namespace gfx {
class RouteSegment {
public:
    RouteSegment();
    RouteSegment(const RouteSegmentOptions& routeOptions, const RouteSegmentHint& rsh);
    void update(const RouteSegmentOptions& routeOptions);
    RouteSegmentOptions getRouteOptions() const;
    RouteSegmentHint getRouteSegmentHint() const;
    ~RouteSegment();

private:
    RouteSegmentOptions options_;
    RouteSegmentHint segmentHint_;

};
} // namespace gfx
} // namespace mbgl
