#include <valarray>
#include <mbgl/route/route.hpp>
#include <mbgl/util/math.hpp>

#include <mbgl/route/route_segment.hpp>
#include <mbgl/util/logging.hpp>

namespace mbgl {

namespace route {

namespace {
const double EPSILON = 1e-8;
}

Route::Route(const LineString<double>& geometry, const RouteOptions& ropts)
    : routeOptions_(ropts),
      geometry_(geometry) {
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
    // regenerate the gradients
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

std::map<double, mbgl::Color> Route::getRouteSegmentColorStops(const mbgl::Color& routeColor) {
    std::map<double, mbgl::Color> colorStops;

    if (segments_.empty()) {
        return getRouteColorStops(routeColor);
    }

    // Initialize the color ramp with the routeColor
    colorStops[0.0] = routeColor;
    colorStops[1.0] = routeColor;

    for (const auto& segment : segments_) {
        const auto& normalizedPositions = segment.getNormalizedPositions();
        const auto& segmentColor = segment.getRouteSegmentOptions().color;

        for (size_t i = 0; i < normalizedPositions.size(); i++) {
            const auto& pos = normalizedPositions[i];
            if(i == 0) {
                double validpos = pos-EPSILON < 0.0 ? 0.0 : pos-EPSILON;
                colorStops[validpos] = routeColor;
            }

            colorStops[pos] = segmentColor;

            if(i == normalizedPositions.size() - 1) {
                double validpos = pos+EPSILON > 1.0f ? 1.0f : pos+EPSILON;
                colorStops[validpos] = routeColor;
            }
        }
    }

    return colorStops;
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
        double distance = std::sqrt(std::pow(progressPoint.x - closestX, 2) + std::pow(progressPoint.y - closestY, 2));

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
    segGradient_.clear();

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
    return *this;
}

} // namespace route
} // namespace mbgl
