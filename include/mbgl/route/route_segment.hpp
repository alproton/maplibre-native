
#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <memory>

#pragma once

namespace mbgl {
namespace route {

struct RouteSegmentOptions {
    Color color = Color(1.0f, 1.f, 1.0f, 1.0f);
    Color outerColor = Color(1.0f, 0.f, 0.0f, 1.0f);
    LineString<double> geometry;
    uint32_t priority = 0;
};

class RouteSegment {
public:
    RouteSegment() = delete;
    RouteSegment(const RouteSegmentOptions& routeSegOptions, const std::vector<double>& normalizedPositions);
    std::vector<double> getNormalizedPositions() const;
    RouteSegmentOptions getRouteSegmentOptions() const;

    ~RouteSegment();

private:
    RouteSegmentOptions options_;
    std::vector<double> normalizedPositions_;
};
} // namespace route
} // namespace mbgl
