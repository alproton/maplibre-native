#include <boost/qvm/mat_operations.hpp>
#include <mbgl/util/math.hpp>
#include <mbgl/util/geometry.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/style/expression/dsl.hpp>

namespace mbgl {
namespace route {

RouteSegment::RouteSegment(const RouteSegmentOptions& routeSegOptions, const std::vector<double>& normalizedPositions)
    : options_(routeSegOptions) {
    normalizedPositions_ = normalizedPositions;
}

std::vector<double> RouteSegment::getNormalizedPositions() const {
    return normalizedPositions_;
}

RouteSegmentOptions RouteSegment::getRouteSegmentOptions() const {
    return options_;
}

RouteSegment::~RouteSegment() {}
} // namespace route
} // namespace mbgl
