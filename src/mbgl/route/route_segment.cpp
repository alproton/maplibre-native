#include <mbgl/util/math.hpp>
#include <mbgl/util/geometry.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/style/expression/dsl.hpp>

namespace mbgl {
namespace route {

template <typename T>
bool point_2_is_between_point_1_and_point_3(const Point<double>& pt1,
                                            const Point<double>& pt2,
                                            const Point<double>& pt3) {
    if (pt1 == pt3) {
        return false;
    } else if((pt2 == pt1) || (pt2 == pt3)) {
        return true;
    }
    else if (pt1.x != pt3.x) {
        return (pt2.x > pt1.x) == (pt2.x < pt3.x);
    } else {
        return (pt2.y > pt1.y) == (pt2.y < pt3.y);
    }
}

RouteSegment::RouteSegment(const RouteSegmentOptions& routeSegOptions, const LineString<double>& routeGeometry, const std::vector<double>& routeGeomDistances, double routeTotalDistance)
    : options_(routeSegOptions) {
    //calculate the normalized points and the expressions
    double currDist = 0.0;
    for(const auto& pt : routeSegOptions.geometry) {
        for(size_t i = 1; i < routeGeometry.size(); ++i) {
            const mbgl::Point<double>& pt1 = routeGeometry[i-1];
            const mbgl::Point<double>& pt2 = pt;
            const mbgl::Point<double>& pt3 = routeGeometry[i];

            if(point_2_is_between_point_1_and_point_3<double>(pt1, pt2, pt3)) {
                double partialDist = mbgl::util::dist<double>(pt1, pt2);
                currDist += partialDist;
                break;
            } else {
                currDist += routeGeomDistances[i-1];
            }
        }
        double normalizedDist = currDist / routeTotalDistance;
        normalizedPositions_.push_back(normalizedDist);

        std::sort(normalizedPositions_.begin(), normalizedPositions_.end());

        currDist = 0.0;
    }
}

const std::vector<double> RouteSegment::getNormalizedPositions() const {
    return normalizedPositions_;
}

RouteSegmentOptions RouteSegment::getRouteSegmentOptions() const {
    return options_;
}

uint32_t RouteSegment::getSortOrder() const {
    return sortOrder_;
}

RouteSegment::~RouteSegment() {}
} // namespace route
} // namespace mbgl
