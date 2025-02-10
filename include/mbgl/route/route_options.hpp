
#pragma once

#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <string>

namespace mbgl {
namespace route {


//enum class RouteSegmentHint {
//    TrafficSlow,
//    TrafficStopAndGo,
//    TrafficStopped,
//    TrafficRoadClosed,
//    Base,
//    Alternative,
//    Invalid
//};

struct RouteSegmentOptions {
    static const std::string BASE_ROUTE_SEGMENT_STR ;

    Color color = Color(1.0f, 1.f, 1.0f, 1.0f);
    LineString<double> lineString;
    uint32_t sortOrder = 0;
    std::string name;
};
} // namespace gfx
} // namespace mbgl
