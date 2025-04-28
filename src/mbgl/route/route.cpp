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

std::string tabs(uint32_t tabcount) {
    std::string tabstr;
    for (size_t i = 0; i < tabcount; ++i) {
        tabstr += "\t";
    }
    return tabstr;
};

[[maybe_unused]] std::string toString(const RouteSegmentOptions& rsopts,
                                      const std::vector<double> normalizedPositions,
                                      uint32_t tabcount) {
    std::stringstream ss;

    ss << tabs(tabcount) << "{" << std::endl;
    ss << tabs(tabcount + 1) << "\"route_segment_options\" : {" << std::endl;
    ss << tabs(tabcount + 2) << "\"color\" : [" << std::to_string(rsopts.color.r) << ", "
       << std::to_string(rsopts.color.g) << ", " << std::to_string(rsopts.color.b) << ", "
       << std::to_string(rsopts.color.a) << "]," << std::endl;
    ss << tabs(tabcount + 2) << "\"priority\" : " << std::to_string(rsopts.priority) << "," << std::endl;
    ss << tabs(tabcount + 2) << "\"geometry\" : [" << std::endl;
    for (size_t i = 0; i < rsopts.geometry.size(); i++) {
        mbgl::Point<double> pt = rsopts.geometry[i];
        std::string terminating = i == rsopts.geometry.size() - 1 ? "" : ",";
        ss << tabs(tabcount + 3) << "[" << std::to_string(pt.x) << ", " << std::to_string(pt.y) << "]" << terminating
           << std::endl;
    }
    ss << tabs(tabcount + 2) << "]," << std::endl; // end of options

    ss << tabs(tabcount + 2) << "\"normalized_positions\" : [";
    for (size_t i = 0; i < normalizedPositions.size(); i++) {
        std::string terminating = i == normalizedPositions.size() - 1 ? "]\n" : " ,";
        ss << std::to_string(normalizedPositions[i]) << terminating;
    }
    ss << tabs(tabcount + 1) << "}" << std::endl;
    ss << tabs(tabcount) << "}";

    return ss.str();
}

} // namespace

Route::Route(const LineString<double>& geometry, const RouteOptions& ropts)
    : routeOptions_(ropts),
      geometry_(geometry) {
    assert(!geometry_.empty() && "route geometry cannot be empty");
    assert((!std::isnan(geometry_[0].x) && !std::isnan(geometry_[0].y)) && "invalid geometry point");
    for (size_t i = 1; i < geometry_.size(); ++i) {
        mbgl::Point<double> a = geometry_[i];
        mbgl::Point<double> b = geometry_[i - 1];
        assert((!std::isnan(a.x) && !std::isnan(a.y)) && "invalid geometry point");
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

bool Route::routeSegmentCreate(const RouteSegmentOptions& rsegopts) {
    RouteSegment routeSeg(rsegopts, geometry_, segDistances_, totalDistance_);
    // client may send invalid segment geometry that is not on the route at all. only add valid segments with segment
    // positions lying on the route.
    if (routeSeg.getNormalizedPositions().empty()) return false;

    segments_.push_back(routeSeg);
    // regenerate the gradients
    segGradient_.clear();

    return true;
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

std::vector<Route::SegmentRange> Route::compactSegments(const RouteType& routeType) const {
    std::vector<RouteSegment> segments = segments_;

    std::sort(segments.begin(), segments.end(), [](const RouteSegment& a, const RouteSegment& b) {
        assert(!a.getNormalizedPositions().empty() && !b.getNormalizedPositions().empty());

        if (!a.getNormalizedPositions().empty() && !b.getNormalizedPositions().empty()) {
            return a.getNormalizedPositions()[0] < b.getNormalizedPositions()[0];
        } else {
            Log::Error(Event::Route, "Route::compactSegments() : Invalid route segment positions");
        }

        return false;
    });

    std::vector<SegmentRange> compacted;
    // insert the first range
    SegmentRange sr;
    double firstPos = segments[0].getNormalizedPositions()[0];
    double lastPos = segments[0].getNormalizedPositions()[segments[0].getNormalizedPositions().size() - 1];
    sr.range = {firstPos, lastPos};
    sr.color = routeType == RouteType::Inner ? segments[0].getRouteSegmentOptions().color
                                             : segments[0].getRouteSegmentOptions().outerColor;
    compacted.push_back(sr);

    for (size_t i = 1; i < segments.size(); i++) {
        const std::vector<double>& prevPositions = segments[i - 1].getNormalizedPositions();
        const std::vector<double>& currPositions = segments[i].getNormalizedPositions();
        const RouteSegmentOptions& prevOptions = segments[i - 1].getRouteSegmentOptions();
        const RouteSegmentOptions& currOptions = segments[i].getRouteSegmentOptions();

        const auto& prevDist = prevPositions[prevPositions.size() - 1];
        const auto& currDist = currPositions[0];
        const auto& prevColor = routeType == RouteType::Inner ? prevOptions.color : prevOptions.outerColor;
        const auto& currColor = routeType == RouteType::Inner ? currOptions.color : prevOptions.outerColor;
        bool isIntersecting = prevDist >= currDist;
        if (isIntersecting) {
            if (prevColor == currColor) {
                // merge the segments
                compacted.rbegin()->range.second = currPositions[currPositions.size() - 1];
                continue;

            } else if (prevOptions.priority >= currOptions.priority) {
                firstPos = prevPositions[prevPositions.size() - 1] + EPSILON;
                lastPos = currPositions[currPositions.size() - 1];
                sr.range = {firstPos, lastPos};
                sr.color = currColor;

            } else if (prevOptions.priority < currOptions.priority) {
                // modify the previous segment and leave some space. We want all segments to be disjoint.
                compacted.rbegin()->range.second = currPositions[0] - EPSILON;

                // add the current segment
                firstPos = currPositions[0];
                lastPos = currPositions[currPositions.size() - 1];
                sr.range = {firstPos, lastPos};
                sr.color = currColor;
            }
        } else {
            firstPos = currPositions[0];
            lastPos = currPositions[currPositions.size() - 1];
            sr.range = {firstPos, lastPos};
            sr.color = currColor;
        }

        compacted.push_back(sr);
    }

    return compacted;
}

std::map<double, mbgl::Color> Route::getRouteSegmentColorStops(const RouteType& routeType,
                                                               const mbgl::Color& routeColor) {
    std::map<double, mbgl::Color> colorStops;
    if (segments_.empty()) {
        return getRouteColorStops(routeColor);
    }

    std::vector<SegmentRange> compacted = compactSegments(routeType);
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

void Route::routeSetProgress(const double t, bool capture) {
    if (capture) {
        capturedNavPercent_.push_back(t);
    }
    progress_ = t;
}

double Route::routeGetProgress() const {
    return progress_;
}

RouteProjectionResult Route::getProgressProjection(const Point<double>& progressPoint, bool capture) {
    RouteProjectionResult result;
    result.success = false;

    if (geometry_.size() < 2) {
        return result; // Cannot form segments
    }

    double totalRouteLength = 0.0;
    std::vector<double> segmentLengths;
    segmentLengths.reserve(geometry_.size() - 1);

    // Calculate total length and individual segment lengths
    for (size_t i = 0; i < geometry_.size() - 1; ++i) {
        double segLen = mbgl::util::dist<double>(geometry_[i], geometry_[i + 1]);
        segmentLengths.push_back(segLen);
        totalRouteLength += segLen;
    }

    // Handle zero-length route
    if (totalRouteLength <= std::numeric_limits<double>::epsilon()) {
        // If the route has no length, the closest point is the first point,
        // and percentage is arguably 0 or undefined. We'll return 0.
        result.closestPoint = geometry_[0];
        result.percentageAlongRoute = 0.0;
        result.success = true; // Technically successful, though degenerate case
        // Check if query point *is* the single point location
        if (mbgl::util::dist<double>(geometry_[0], progressPoint) > std::numeric_limits<double>::epsilon()) {
            // If query point is different, maybe success should be false? Depends on requirements.
            // Let's keep it true but maybe add a warning/note.
            Log::Debug(Event::Route, "Warning: Route has zero total length. Closest point set to route start.");
        }
        return result;
    }

    double minDistanceSq = std::numeric_limits<double>::max();
    double distanceToBestSegmentStart = 0.0; // Accumulates distance up to the start of the 'best' segment
    double distanceAlongBestSegment = 0.0;

    double currentDistanceAlongRoute = 0.0; // Accumulates distance as we iterate
                                            // Iterate through each segment of the route
    for (size_t i = 0; i < geometry_.size() - 1; ++i) {
        const mbgl::Point<double>& p1 = geometry_[i];     // Start point of the segment
        const mbgl::Point<double>& p2 = geometry_[i + 1]; // End point of the segment
        double currentSegmentLength = segmentLengths[i];

        mbgl::Point<double> segmentVector = p2 - p1;
        mbgl::Point<double> queryVector = progressPoint - p1;

        mbgl::Point<double> closestPointOnSegment;
        double distAlongCurrentSegment = 0.0;

        // Calculate the squared length of the segment vector
        double segmentLenSq = mbgl::util::distSqr<double>(
            p1, p2); // segmentVector.x * segmentVector.x + segmentVector.y * segmentVector.y;

        if (segmentLenSq < std::numeric_limits<double>::epsilon()) {
            // Segment has zero length, closest point on segment is just p1 (or p2)
            closestPointOnSegment = p1;
            distAlongCurrentSegment = 0.0; // No distance along a zero-length segment
        } else {
            // Project queryVector onto segmentVector
            // t = dot(queryVector, segmentVector) / dot(segmentVector, segmentVector)
            double dotProduct = queryVector.x * segmentVector.x + queryVector.y * segmentVector.y;
            double t = dotProduct / segmentLenSq;

            // Clamp t to the range [0, 1] to stay within the segment
            if (t < 0.0) {
                closestPointOnSegment = p1; // Closest point is the start of the segment
                distAlongCurrentSegment = 0.0;
            } else if (t > 1.0) {
                closestPointOnSegment = p2;                     // Closest point is the end of the segment
                distAlongCurrentSegment = currentSegmentLength; // Full length of this segment
            } else {
                // Projection lies within the segment
                closestPointOnSegment = p1 + (segmentVector * t);
                // Calculate distance from p1 to the projected point
                distAlongCurrentSegment = mbgl::util::dist<double>(p1, closestPointOnSegment);
            }
        }

        // Check if the closest point on this segment is the closest overall found so far
        double currentDistSq = mbgl::util::distSqr<double>(progressPoint, closestPointOnSegment);
        if (currentDistSq < minDistanceSq) {
            minDistanceSq = currentDistSq;
            result.closestPoint = closestPointOnSegment;
            distanceToBestSegmentStart = currentDistanceAlongRoute; // Store distance up to the start of this segment
            distanceAlongBestSegment = distAlongCurrentSegment;     // Store distance along this segment
            result.success = true;
        }

        // Accumulate distance for the next iteration
        currentDistanceAlongRoute += currentSegmentLength;
    }

    // Calculate the final percentage
    if (result.success) {
        double totalDistanceToClosestPoint = distanceToBestSegmentStart + distanceAlongBestSegment;
        result.percentageAlongRoute = (totalDistanceToClosestPoint / totalRouteLength);

        // Clamp percentage just in case of floating point inaccuracies near ends
        if (result.percentageAlongRoute < 0.0) result.percentageAlongRoute = 0.0;
        if (result.percentageAlongRoute > 1.0) result.percentageAlongRoute = 1.0;
        if (capture) {
            capturedNavStops_.push_back(result.closestPoint);
        }
    }

    return result;
}

const std::vector<Point<double>>& Route::getCapturedNavStops() const {
    return capturedNavStops_;
}

const std::vector<double>& Route::getCapturedNavPercent() const {
    return capturedNavPercent_;
}

double Route::getProgressPercent(const Point<double>& progressPoint, bool capture) {
    if (capture) {
        capturedNavStops_.push_back(progressPoint);
    }

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

std::string Route::segmentsToString(uint32_t tabcount) const {
    std::stringstream ss;
    ss << tabs(tabcount) << "[" << std::endl;
    for (size_t i = 0; i < segments_.size(); i++) {
        const auto& segment = segments_[i];
        std::string terminatingCommaStr = i == segments_.size() - 1 ? "" : ",";
        ss << toString(segment.getRouteSegmentOptions(), segment.getNormalizedPositions(), tabcount + 1)
           << terminatingCommaStr << std::endl;
    }
    ss << tabs(tabcount) << "]";

    return ss.str();
}

} // namespace route
} // namespace mbgl
