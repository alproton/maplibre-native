

#include <valarray>
#include <mbgl/route/route.hpp>
#include <mbgl/util/math.hpp>

#include <mbgl/route/route_segment.hpp>

namespace mbgl {

namespace route {

Route::Route(const LineString<double>& geometry, const RouteOptions& ropts)
    : routeOptions_(ropts), geometry_(geometry) {
    for (size_t i = 1; i < geometry_.size(); ++i) {
        mbgl::Point<double> a = geometry_[i];
        mbgl::Point<double> b = geometry_[i - 1];
        double dist = std::abs(mbgl::util::dist<double>(a, b));
        segDistances_.push_back(dist);
        totalDistance_ += dist;
    }
}

const RouteOptions& Route::getRouteOptions() const {
    return routeOptions_;
}

double Route::getTotalDistance() const {
    return totalDistance_;
}

bool Route::hasRouteSegments() const {
    return !segments_.empty();
}

void Route::routeSegmentCreate(const RouteSegmentOptions& rsegopts) {
    RouteSegment routeSeg(rsegopts, geometry_, segDistances_, totalDistance_);
    segments_.push_back(routeSeg);
    //regenerate the gradients
    segGradient_.clear();
}

mbgl::LineString<double> Route::getGeometry() const {
    return geometry_;
}

std::map<double, mbgl::Color> Route::getRouteColorStops(const mbgl::Color& routeColor) const {
    std::map<double, mbgl::Color> gradients;
        gradients[0.0] = routeColor;
        gradients[1.0] = routeColor;

    return gradients;
}

// std::map<double, mbgl::Color> Route::getRouteSegmentColorStops(const mbgl::Color& routeColor) {
//     if (segments_.empty()) {
//         return getRouteColorStops(routeColor);
//     }
//     if (segGradient_.empty()) {
//         std::sort(segments_.begin(), segments_.end(), [](const RouteSegment& lhs, const RouteSegment& rhs) {
//             return lhs.getRouteSegmentOptions().sortOrder <= rhs.getRouteSegmentOptions().sortOrder;
//         });
//         const double EPSILON = 0.00001;
//
//         //simply bake the colors
//         for (size_t i = 0; i < segments_.size(); ++i) {
//             const auto& segNormalizedPos = segments_[i].getNormalizedPositions();
//             const auto& segColor = segments_[i].getRouteSegmentOptions().color;
//
//             for (size_t j = 0; j < segNormalizedPos.size(); ++j) {
//                 const auto& segPos = segNormalizedPos[j];
//
//                 if(i == 0 && j == 0) {
//                     if(segPos <  EPSILON) {
//                         segGradient_[0.0] = segColor;
//                     } else {
//                         segGradient_[0.0] = routeColor;
//                     }
//                     continue;
//
//                 }
//
//                 size_t segmentSz = segments_.size() - 1;
//                 if(i == segments_.size() - 1 && j == segmentSz) {
//                     if(segPos > 1 - EPSILON) {
//                         segGradient_[1.0] = segColor;
//                     } else {
//                         segGradient_[1.0] = routeColor;
//                     }
//                 }
//
//                 segGradient_[segPos] = segColor;
//             }
//         }
//     }
//
//     return segGradient_;
// }

std::map<double, mbgl::Color> Route::getRouteSegmentColorStops(const mbgl::Color& routeColor) {
    const double EPSILON = 0.00001;
    if (segments_.empty()) {
        return getRouteColorStops(routeColor);
    }
    std::sort(segments_.begin(), segments_.end(), [](const RouteSegment& lhs, const RouteSegment& rhs) {
        return lhs.getRouteSegmentOptions().sortOrder <= rhs.getRouteSegmentOptions().sortOrder;
    });
    if (segGradient_.empty()) {
        for (size_t i = 0; i < segments_.size(); ++i) {
            const auto& segNormalizedPos = segments_[i].getNormalizedPositions();
            const auto& segColor = segments_[i].getRouteSegmentOptions().color;

            // gradients need to start at 0.0 and end with 1.0.
            // each route segment is padded with route color for a small transition gradient.
            for (size_t j = 0; j < segNormalizedPos.size(); ++j) {
                const auto& segPos = segNormalizedPos[j];

                if ((i == 0 && j == 0) || (i == segments_.size() - 1 && j == segNormalizedPos.size() - 1)) {
                    continue;
                }

                if (j == 0) {
                    segGradient_[segPos - EPSILON] = routeColor;
                }

                segGradient_[segPos] = segColor;

                if (j == segNormalizedPos.size() - 1) {
                    segGradient_[segPos + EPSILON] = routeColor;
                }
            }
        }

        const auto& firstSegNormalizedPos = segments_[0].getNormalizedPositions();
        const auto& lastSegNormalizedPos = segments_[segments_.size() - 1].getNormalizedPositions();
        const auto& firstSegColor = segments_[0].getRouteSegmentOptions().color;
        const auto& lastSegColor = segments_[segments_.size() - 1].getRouteSegmentOptions().color;

        assert(firstSegNormalizedPos[0] >= 0.0 && "normalized positions cannot be < 0.0");
        double firstNpos = firstSegNormalizedPos[0];
        if (firstNpos < EPSILON) {
            segGradient_[0.0] = firstSegColor;
        } else {
            segGradient_[0.0] = routeColor;

            segGradient_[firstNpos - EPSILON] = routeColor;
            segGradient_[firstNpos] = firstSegColor;
        }

        if (lastSegNormalizedPos[lastSegNormalizedPos.size() - 1] >= (1.0 - EPSILON)) {
            segGradient_[1.0] = lastSegColor;
        } else {
            double lastNpos = lastSegNormalizedPos[lastSegNormalizedPos.size() - 1];
            segGradient_[lastNpos] = lastSegColor;
            segGradient_[lastNpos + EPSILON] = routeColor;
            segGradient_[1.0] = routeColor;
        }
    }

    return segGradient_;
}

void Route::routeSetProgress(const double t) {
    progress_ = t;
}

double Route::routeGetProgress() const {
    return progress_;
}

double Route::getProgressPercent(const Point<double>& progressPoint) const {
    if (geometry_.size() < 2) {
        return 0.0;
    }

    double minDistance = std::numeric_limits<double>::max();
    size_t closestSegmentIndex = 0;
    double closestPercentage = 0.0;

    // Iterate through each line segment in the polyline
    for (size_t i = 0; i < geometry_.size() - 1; i++) {
        const Point<double>& p1 = geometry_[i];
        const Point<double>& p2 = geometry_[i + 1];

        // Vector from p1 to p2
        double segmentX = p2.x - p1.x;
        double segmentY = p2.y - p1.y;
        double segmentLength = segDistances_[i];

        if (segmentLength == 0.0) continue;

        // Vector from p1 to progressPoint
        double vectorX = progressPoint.x - p1.x;
        double vectorY = progressPoint.y - p1.y;

        // Project progressPoint onto the line segment
        double projection = (vectorX * segmentX + vectorY * segmentY) / segmentLength;
        double percentage = std::max(0.0, std::min(1.0, projection / segmentLength));

        // Calculate the closest point on the line segment
        double closestX = p1.x + percentage * segmentX;
        double closestY = p1.y + percentage * segmentY;

        // Calculate distance to the line segment
        double distance = std::sqrt(
            std::pow(progressPoint.x - closestX, 2) +
            std::pow(progressPoint.y - closestY, 2)
        );

        // Update if this is the closest segment found
        if (distance < minDistance) {
            minDistance = distance;
            closestSegmentIndex = i;
            closestPercentage = percentage;
        }
    }

    // Calculate distance up to the projected point using stored segment distances
    double distanceToPoint = 0.0;
    for (size_t i = 0; i < closestSegmentIndex; i++) {
        distanceToPoint += segDistances_[i];
    }
    distanceToPoint += segDistances_[closestSegmentIndex] * closestPercentage;

    return totalDistance_ > 0.0 ? distanceToPoint / totalDistance_ : 0.0;
}

uint32_t Route::getNumRouteSegments() const {
    return static_cast<uint32_t>(segments_.size());
}

std::vector<double> Route::getRouteSegmentDistances() const {
    return segDistances_;
}

bool Route::routeSegmentsClear() {
    segments_.clear();

    return true;
}

Route& Route::operator=(Route& other) noexcept {
    if (this == &other) {
        return *this;
    }
    routeOptions_ = other.routeOptions_;
    progress_ = other.progress_;
    segDistances_ = other.segDistances_;
    segments_ = other.segments_;
    geometry_ = other.geometry_;
    totalDistance_ = other.totalDistance_;
    segGradient_ = other.segGradient_;
    routeSegTransitionDist_ = other.routeSegTransitionDist_;
    progressColor_ = other.progressColor_;
    return *this;
}

} // namespace route
} // namespace mbgl