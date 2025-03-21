#include "mbgl/programs/attributes.hpp"
#include "mbgl/programs/uniforms.hpp"

#include <valarray>
#include <mbgl/route/route.hpp>
#include <mbgl/util/math.hpp>

#include <mbgl/route/route_segment.hpp>
#include <mbgl/util/logging.hpp>

namespace mbgl {

namespace route {

namespace {
const double EPSILON = 1e-8;
const double HALF_EPSILON = EPSILON * 0.5;
} // namespace

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

std::vector<Route::SegmentRange> Route::compactSegments() const {
    // TODO: perhaps we should use a std::set instead of an std::vector to store the sorted segments
    std::vector<RouteSegment> segments = segments_;
    std::sort(segments.begin(), segments.end(), [](const RouteSegment& a, const RouteSegment& b) {
        return a.getNormalizedPositions()[0] < b.getNormalizedPositions()[0];
    });

    std::vector<SegmentRange> compacted;
    // insert the first range
    SegmentRange sr;
    double firstPos = segments[0].getNormalizedPositions()[0];
    double lastPos = segments[0].getNormalizedPositions()[segments[0].getNormalizedPositions().size() - 1];
    sr.range = std::make_pair<double, double>(std::move(firstPos), std::move(lastPos));
    sr.color = segments[0].getRouteSegmentOptions().color;
    compacted.push_back(sr);

    for (size_t i = 1; i < segments.size(); i++) {
        const std::vector<double>& prevPositions = segments[i - 1].getNormalizedPositions();
        const std::vector<double>& currPositions = segments[i].getNormalizedPositions();
        const RouteSegmentOptions& prevOptions = segments[i - 1].getRouteSegmentOptions();
        const RouteSegmentOptions& currOptions = segments[i].getRouteSegmentOptions();

        const auto& prevDist = prevPositions[prevPositions.size() - 1];
        const auto& currDist = currPositions[0];
        const auto& prevColor = prevOptions.color;
        const auto& currColor = currOptions.color;
        bool isIntersecting = prevDist > currDist;
        if (isIntersecting) {
            if (prevColor == currColor) {
                // merge the segments
                compacted.rbegin()->range.second = currPositions[currPositions.size() - 1];
                continue;

            } else if (prevOptions.priority >= currOptions.priority) {
                firstPos = prevPositions[prevPositions.size() - 1] + EPSILON;
                lastPos = currPositions[currPositions.size() - 1];
                sr.range = std::make_pair(firstPos, lastPos);
                sr.color = currColor;

            } else if (prevOptions.priority < currOptions.priority) {
                // modify the previous segment and leave some space. We want all segments to be disjoint.
                compacted.rbegin()->range.second = currPositions[0] - EPSILON;

                // add the current segment
                firstPos = currPositions[0];
                lastPos = currPositions[currPositions.size() - 1];
                sr.range = std::make_pair(firstPos, lastPos);
                sr.color = currColor;
            }
        } else {
            firstPos = currPositions[0];
            lastPos = currPositions[currPositions.size() - 1];
            sr.range = std::make_pair(firstPos, lastPos);
            sr.color = currColor;
        }

        compacted.push_back(sr);
    }

    return compacted;
}

std::map<double, mbgl::Color> Route::getRouteSegmentColorStops(const mbgl::Color& routeColor) {
    std::map<double, mbgl::Color> colorStops;
    if (segments_.empty()) {
        return getRouteColorStops(routeColor);
    }

    std::vector<SegmentRange> compacted = compactSegments();
    // Initialize the color ramp with the routeColor
    colorStops[0.0] = routeColor;

    for (const auto& sr : compacted) {
        double firstPos = sr.range.first;
        double lastPos = sr.range.second;

        double pre_pos = firstPos - HALF_EPSILON < 0.0 ? 0.0 : firstPos - HALF_EPSILON;
        double post_pos = lastPos + HALF_EPSILON > 1.0f ? 1.0f : lastPos + HALF_EPSILON;

        colorStops[pre_pos] = routeColor;
        colorStops[firstPos] = sr.color;
        colorStops[lastPos] = sr.color;
        colorStops[post_pos] = routeColor;
    }

    if (colorStops.rbegin()->first != 1.0) {
        colorStops[1.0] = routeColor;
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
