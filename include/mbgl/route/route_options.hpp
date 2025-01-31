
//
// Created by spalaniappan on 1/6/25.
//

#pragma once

#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>

namespace mbgl {
namespace gfx {
enum class RouteSegmentHint {
    TrafficSlow,
    TrafficStopAndGo,
    TrafficStopped,
    TrafficRoadClosed,
    Base,
    Alternative,
    Invalid
};

struct RouteSegmentOptions {
    Color innerColor = Color(1.0f, 1.f, 1.0f, 1.0f);
    Color outerColor;
    LineString<double> lineString;
    uint32_t sortOrder = 0;

};
} // namespace gfx
} // namespace mbgl
