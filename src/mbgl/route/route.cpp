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

#define INVALID_UINT uint32_t(~0)

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
    ss << tabs(tabcount + 2) << "\"outer_color\" : [" << std::to_string(rsopts.outerColor.r) << ", "
       << std::to_string(rsopts.outerColor.g) << ", " << std::to_string(rsopts.outerColor.b) << ", "
       << std::to_string(rsopts.outerColor.a) << "]," << std::endl;
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

// --- Constants ---
constexpr double PI = 3.14159265358979323846;

// --- Helper Functions ---

// Converts degrees to radians
inline double degreesToRadians(double degrees) {
    return degrees * PI / 180.0;
}

// Converts radians to degrees
inline double radiansToDegrees(double radians) {
    return radians * 180.0 / PI;
}

struct Vector3D {
    double x;
    double y;
    double z;
};

Vector3D vecSubtract(const Vector3D& a, const Vector3D& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vector3D vecAdd(const Vector3D& a, const Vector3D& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vector3D vecScale(const Vector3D& v, double scalar) {
    return {v.x * scalar, v.y * scalar, v.z * scalar};
}
double vecDot(const Vector3D& a, const Vector3D& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
double vecMagnitudeSq(const Vector3D& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

/**
 * @brief Converts Latitude/Longitude (degrees) to 3D Cartesian coordinates (unit sphere). Latitude ranges from
 * -90 to +90, where +ve is above equator and -ve is below equator. Longitude ranges from -180 to +180, where -ve is
 * is west of prime meridian and +ve is east of prime meridian.
 *
 * @param p Point with latitude and longitude in degrees.
 * @return Vector3D Cartesian coordinates (x, y, z).
 */
Vector3D latLonToCartesian(const Point<double>& p) {
    static constexpr double EARTH_RADIUS_METERS = 6371000.0;
    double latRad = degreesToRadians(p.y);
    double lonRad = degreesToRadians(p.x);

    // scale cartesian coords to earth radius, doing in unit sphere we lose a lot of precision even when using double.
    return {
        EARTH_RADIUS_METERS * std::cos(latRad) * std::cos(lonRad), // x
        EARTH_RADIUS_METERS * std::cos(latRad) * std::sin(lonRad), // y
        EARTH_RADIUS_METERS * std::sin(latRad)                     // z
    };
}

/**
 * @brief Converts 3D Cartesian coordinates (unit sphere) back to Latitude/Longitude (degrees).
 * @param v Cartesian vector (x, y, z). Assumed to be normalized (unit length).
 * @return Point with latitude and longitude in degrees.
 */
Point<double> cartesianToLatLon(const Vector3D& v) {
    // Ensure the vector is normalized (or close enough) for accurate asin
    double length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    assert(length > EPSILON && "Vector length should be non-zero");
    Vector3D vnormalized = {v.x / length, v.y / length, v.z / length};

    // Clamp z to [-1, 1] to avoid domain errors in asin due to potential floating point inaccuracies
    double clamped_z = std::clamp(vnormalized.z, -1.0, 1.0);

    double latitudeRad = std::asin(clamped_z);                      // Latitude from z component
    double longitudeRad = std::atan2(vnormalized.y, vnormalized.x); // Longitude from atan2(y, x)

    return {radiansToDegrees(longitudeRad), radiansToDegrees(latitudeRad)};
}

} // namespace

Route::Route(const LineString<double>& geometry, const RouteOptions& ropts)
    : routeOptions_(ropts),
      geometry_(geometry) {
    assert(!geometry_.empty() && "route geometry cannot be empty");
    assert((!std::isnan(geometry_[0].x) && !std::isnan(geometry_[0].y)) && "invalid geometry point");

    intervalLengths_.reserve(geometry_.size() - 1);
    cumulativeIntervalDistances_.reserve(geometry_.size());
    cumulativeIntervalDistances_.push_back(0.0);

    for (size_t i = 0; i < geometry_.size() - 1; ++i) {
        const mbgl::Point<double>& p1 = geometry_[i];
        const mbgl::Point<double>& p2 = geometry_[i + 1];
        assert((!std::isnan(p1.x) && !std::isnan(p1.y) && !std::isnan(p2.x) && !std::isnan(p2.y)) &&
               "invalid geometry point");
        double segLen = mbgl::util::haversineDist(p1, p2);
        intervalLengths_.push_back(segLen);
        totalLength_ += segLen;
        cumulativeIntervalDistances_.push_back(totalLength_);
    }
}

std::optional<double> Route::getVectorOrientation(double x, double y) {
    // Handle the zero vector case (angle is undefined)
    if (x == 0.0 && y == 0.0) {
        return std::nullopt;
    }

    // Calculate the angle in radians using std::atan2(x, y).
    // std::atan2(x, y) returns an angle where:
    // - Positive Y-axis (North, vector (0,y_pos)) is 0 radians.
    // - Positive X-axis (East, vector (x_pos,0)) is PI/2 radians.
    // - Negative Y-axis (South, vector (0,y_neg)) is PI radians.
    // - Negative X-axis (West, vector (x_neg,0)) is -PI/2 radians.
    // This convention means positive angles are measured clockwise from North.
    double angleRadians = std::atan2(x, y);

    // Convert radians to degrees
    double angleDegrees = angleRadians * (180.0 / M_PI);
    // Normalize the angle to be in the range [0, 360) degrees.
    // std::atan2 returns values in (-PI, PI], so angleDegrees is in (-180, 180].
    if (angleDegrees < 0.0) {
        angleDegrees += 360.0;
    }

    // Ensure that if the angle is exactly 360 (e.g. due to -0.0 input that got normalized),
    // it is represented as 0.0. This also handles the case where angleDegrees might be -0.0
    // which is fine, but for strict [0,360) output where 360 becomes 0.
    if (angleDegrees == 360.0) {
        return 0.0;
    }
    // Handle -0.0 becoming 0.0 for consistent output if desired, though numerically they are same.
    if (angleDegrees == -0.0) {
        return 0.0;
    }

    return angleDegrees;
}

Point<double> Route::getPointCoarse(double percentage, double* bearing) const {
    if (geometry_.empty()) {
        throw std::invalid_argument("Route cannot be empty.");
    }

    if (geometry_.size() == 1) {
        return geometry_[0]; // If only one point, return it regardless of percentage
    }

    // Clamp percentage to the valid range [0.0, 1.0]
    if (percentage < 0.0) percentage = 0.0;
    if (percentage > 1.0) percentage = 1.0;

    if (percentage == 0.0) {
        return geometry_.front();
    }
    if (percentage == 1.0) {
        return geometry_.back();
    }

    // Handle cases where the total distance is zero (e.g., all points are identical)
    if (totalLength_ <= EPSILON) {
        return geometry_.front(); // Return the start point if total distance is negligible
    }

    // Calculate the target distance based on the percentage
    double target_distance = totalLength_ * percentage;

    // Iterate through segments to find the one containing the target distance
    double currCumulativeLen = 0.0;
    for (size_t i = 0; i < intervalLengths_.size(); ++i) {
        double currIntervalLen = intervalLengths_[i];

        // Check if the target point lies within or at the end of the current segment
        // Use a small tolerance for floating point comparison
        if (currCumulativeLen + currIntervalLen >= target_distance - EPSILON) {
            const Point<double>& p1 = geometry_[i];
            const Point<double>& p2 = geometry_[i + 1];

            if (bearing != nullptr) {
                std::optional<double> bearingOpt = getVectorOrientation(p2.x - p1.x, p2.y - p1.y);
                *bearing = bearingOpt.has_value() ? bearingOpt.value() : 0.0;
            }

            // Calculate how far into the current segment the target distance is
            double lenNeededInInterval = target_distance - currCumulativeLen;

            // Calculate the fraction of the current segment needed
            double intervalFraction = 0.0;
            if (currIntervalLen > EPSILON) {
                intervalFraction = lenNeededInInterval / currIntervalLen;
            } else {
                // If segment length is zero, just return the start point of the segment
                return p1;
            }

            // Clamp fraction just in case of floating point issues
            if (intervalFraction < 0.0) intervalFraction = 0.0;
            if (intervalFraction > 1.0) intervalFraction = 1.0;

            // Linear Interpolation (LERP) between the segment's start and end points
            // Note: Simple linear interpolation of lat/lon is an approximation.
            // For very long segments or high accuracy needs, spherical linear
            // interpolation (Slerp) might be preferred, but it's more complex.
            // LERP is often sufficient for typical navigation route segments.
            double resultLon = p1.x + (p2.x - p1.x) * intervalFraction;
            double resultLat = p1.y + (p2.y - p1.y) * intervalFraction;

            return {resultLon, resultLat};
        }

        // Add the current segment's length to the cumulative distance
        currCumulativeLen += currIntervalLen;
    }

    // Should theoretically not be reached if percentage <= 1.0 due to checks,
    // but as a fallback, return the last point (covers percentage = 1.0 edge case
    // potentially missed by floating point issues).
    return geometry_.back();
}

Point<double> Route::getPointFine(double percentage, double* bearing) const {
    if (geometry_.empty()) {
        throw std::invalid_argument("Route cannot be empty.");
    }

    if (geometry_.size() == 1) {
        return geometry_[0]; // If only one point, return it regardless of percentage
    }

    // Clamp percentage to the valid range [0.0, 1.0]
    if (percentage < 0.0) percentage = 0.0;
    if (percentage > 1.0) percentage = 1.0;

    if (percentage == 0.0) {
        return geometry_.front();
    }
    if (percentage == 1.0) {
        return geometry_.back();
    }

    // Handle cases where the total distance is zero (e.g., all points are identical)
    if (totalLength_ <= EPSILON) {
        return geometry_.front(); // Return the start point if total distance is negligible
    }

    // Calculate the target distance based on the percentage
    double targetLen = totalLength_ * percentage;

    // Iterate through segments to find the one containing the target distance
    double cumulativeLen = 0.0;
    for (size_t i = 0; i < intervalLengths_.size(); ++i) {
        double currIntervalLen = intervalLengths_[i];

        // Check if the target point lies within or at the end of the current segment
        // Use a small tolerance for floating point comparison
        if (cumulativeLen + currIntervalLen >= targetLen - EPSILON) {
            const Point<double>& p1 = geometry_[i];
            const Point<double>& p2 = geometry_[i + 1];

            if (bearing != nullptr) {
                std::optional<double> bearingOpt = getVectorOrientation(p2.x - p1.x, p2.y - p1.y);
                *bearing = bearingOpt.has_value() ? bearingOpt.value() : 0.0;
            }

            double distance_needed_in_segment = targetLen - cumulativeLen;
            double intervalFraction = 0.0;

            if (currIntervalLen > EPSILON) {
                intervalFraction = distance_needed_in_segment / currIntervalLen;
            } else {
                // Segment length is effectively zero, return the start point
                return p1;
            }
            intervalFraction = std::clamp(intervalFraction, 0.0, 1.0); // Clamp fraction

            // --- 4. Perform SLERP ---
            Vector3D v1 = latLonToCartesian(p1);
            Vector3D v2 = latLonToCartesian(p2);

            // Dot product -> angle between vectors
            double dot = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;

            // Clamp dot product to avoid acos domain errors due to floating point inaccuracies
            dot = std::clamp(dot, -1.0, 1.0);

            double omega = std::acos(dot); // Angle between v1 and v2 in radians
            double sinOmega = std::sin(omega);

            // Handle cases where points are nearly antipodal (or identical again)
            if (std::abs(sinOmega) < EPSILON) {
                // Points are identical or antipodal.
                // If identical (omega ~ 0), already handled above.
                // If antipodal (omega ~ PI), SLERP is ill-defined (infinite great circles).
                // Falling back to LERP or returning p1 is a pragmatic choice for route segments.
                // Return p1 as a fallback.
                // or
                // Alternatively LERP:
                double result_lat = p1.y + (p2.y - p1.y) * intervalFraction;
                double result_lon = p1.x + (p2.x - p1.x) * intervalFraction;
                return {result_lon, result_lat};
            }

            // SLERP formula components
            double a = std::sin((1.0 - intervalFraction) * omega) / sinOmega;
            double b = std::sin(intervalFraction * omega) / sinOmega;

            // Interpolated Cartesian vector
            Vector3D vResult = {a * v1.x + b * v2.x, a * v1.y + b * v2.y, a * v1.z + b * v2.z};

            // Convert back to Lat/Lon (degrees)
            return cartesianToLatLon(vResult);
            // --- End SLERP ---
        }
        cumulativeLen += currIntervalLen;
    }

    // Should theoretically not be reached if percentage <= 1.0 due to checks,
    // but as a fallback, return the last point (covers percentage = 1.0 edge case
    // potentially missed by floating point issues).
    return geometry_.back();
}

mbgl::Point<double> Route::getPoint(double percentage, const Precision& precision, double* bearing) const {
    switch (precision) {
        case Precision::Coarse:
            return getPointCoarse(percentage, bearing);
        case Precision::Fine:
            return getPointFine(percentage, bearing);
        default:
            throw std::invalid_argument("Invalid precision type.");
    }

    return {};
}

const RouteOptions& Route::getRouteOptions() const {
    return routeOptions_;
}

double Route::getTotalDistance() const {
    return totalLength_;
}

bool Route::hasRouteSegments() const {
    return !segments_.empty();
}

bool Route::routeSegmentCreate(const RouteSegmentOptions& rsegopts) {
    std::vector<double> normalizedPositions;
    normalizedPositions.reserve(rsegopts.geometry.size());
    for (const auto& segpt : rsegopts.geometry) {
        if (std::isnan(segpt.x) || std::isnan(segpt.y)) {
            Log::Error(Event::Route, "Route segment geometry contains NaN point");
            return false;
        }
        double normalized = getProgressPercent(segpt, Precision::Fine, false);
        normalizedPositions.push_back(normalized);
    }
    RouteSegment routeSeg(rsegopts, normalizedPositions);

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
        const auto& currColor = routeType == RouteType::Inner ? currOptions.color : currOptions.outerColor;
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
        if (lastPos < 1.0) {
            colorStops[post_pos] = routeColor;
        }
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

const std::vector<Point<double>>& Route::getCapturedNavStops() const {
    return capturedNavStops_;
}

const std::vector<double>& Route::getCapturedNavPercent() const {
    return capturedNavPercent_;
}

double Route::getProgressPercent(const Point<double>& progressPoint, const Precision& precision, bool capture) {
    double percentage = -1.0;
    switch (precision) {
        case Precision::Coarse: {
            percentage = getProgressProjectionLERP(progressPoint, capture);
            break;
        }
        case Precision::Fine: {
            percentage = getProgressProjectionSLERP(progressPoint, capture);
            break;
        }
        default:
            throw std::invalid_argument("Invalid precision type.");
    }

    if (logPrecision) {
        mbgl::Point<double> calculatedPt = getPointFine(percentage);
        double dist = mbgl::util::haversineDist(progressPoint, calculatedPt);
        std::string precisionStr = precision == Precision::Coarse ? "Coarse" : "Fine";
        std::string msg = precisionStr + ": progressPoint: " + std::to_string(progressPoint.x) + " " +
                          std::to_string(progressPoint.y) + ", calculatePt: " + std::to_string(calculatedPt.x) + " " +
                          std::to_string(calculatedPt.y) + " ,error rate distance: " + std::to_string(dist);
        Log::Debug(Event::Route, msg);
    }

    return percentage;
}

double Route::getProgressProjectionLERP(const Point<double>& queryPoint, bool capture) {
    if (capture) {
        capturedNavStops_.push_back(queryPoint);
    }

    if (geometry_.size() < 2) {
        return -1.0; // Cannot form segments
    }

    // Handle zero-length route
    if (totalLength_ <= EPSILON) {
        // If the route has no length, the closest point is the first point,
        // and percentage is arguably 0 or undefined. We'll return 0.
        // Check if query point *is* the single point location
        if (mbgl::util::haversineDist(geometry_[0], queryPoint) > EPSILON) {
            // If query point is different, maybe success should be false? Depends on requirements.
            // Let's keep it true but maybe add a warning/note.
            Log::Debug(Event::Route, "Warning: Route has zero total length. Closest point set to route start.");
        }
        return 0.0;
    }

    const auto& getClosestInterval = [&](uint32_t startIdx, uint32_t endIdx) -> std::pair<uint32_t, double> {
        uint32_t bestIntervalIndex = INVALID_UINT;
        double bestIntervalFraction = -1.0;
        double minDistanceSq = std::numeric_limits<double>::max();

        for (size_t i = startIdx; i <= endIdx; ++i) {
            const mbgl::Point<double>& p1 = geometry_[i];     // Start point of the segment
            const mbgl::Point<double>& p2 = geometry_[i + 1]; // End point of the segment

            // --- Project queryPoint onto the 2D line segment (p1, p2) ---
            // Treat Lat/Lon as simple 2D coordinates for projection (approximation)
            double segmentDX = p2.x - p1.x; // longitude
            double segmentDY = p2.y - p1.y; // lattitude

            double segmentLenSq = segmentDX * segmentDX + segmentDY * segmentDY;

            double t = 0.0; // Parameter along the line segment (0=p1, 1=p2)
            Point<double> closestPointOnSegment = p1;

            if (segmentLenSq > EPSILON) {
                // Project (queryPoint - p1) onto (p2 - p1)
                double queryPointDX = queryPoint.x - p1.x;
                double queryPointDY = queryPoint.y - p1.y;
                t = (queryPointDX * segmentDX + queryPointDY * segmentDY) / segmentLenSq;
                t = std::clamp(t, 0.0, 1.0); // Clamp to the segment bounds [0, 1]

                // Calculate the point on the segment corresponding to clamped t
                closestPointOnSegment.y = p1.y + t * segmentDY;
                closestPointOnSegment.x = p1.x + t * segmentDX;
            } else {
                // Segment has zero length, closest point is p1 (or p2, they are the same)
                // t remains 0.0;
                closestPointOnSegment = p1;
            }

            // --- Calculate distance from queryPoint to this closest point ---
            // IMPORTANT: Use Haversine distance for the actual distance check,
            // even though projection used linear approximation.
            double dist = mbgl::util::haversineDist(queryPoint, closestPointOnSegment);
            double distSq = dist * dist; // Compare squared distances to avoid sqrt

            if (distSq < minDistanceSq) {
                minDistanceSq = distSq;
                bestIntervalIndex = i;
                bestIntervalFraction = t; // Use the clamped fraction
            }
        }

        return {bestIntervalIndex, bestIntervalFraction};
    };

    // closestInterval.first is the interval index, closestInterval.second is the fraction along the interval
    std::pair<uint32_t, double> closestInterval = getClosestInterval(0, geometry_.size() - 1);

    // --- Calculate final percentage ---
    uint32_t bestIntervalIndex = closestInterval.first;
    double bestIntervalFraction = closestInterval.second;
    double distanceAlongRoute = cumulativeIntervalDistances_[bestIntervalIndex] +
                                (bestIntervalFraction * intervalLengths_[bestIntervalIndex]);

    return distanceAlongRoute / totalLength_;
}

double Route::getProgressProjectionSLERP(const Point<double>& queryPoint, bool capture) {
    if (capture) {
        capturedNavStops_.push_back(queryPoint);
    }

    if (geometry_.size() < 2) {
        return -1.0; // Cannot form segments
    }

    // Handle zero-length route
    if (totalLength_ <= EPSILON) {
        // If the route has no length, the closest point is the first point,
        // and percentage is arguably 0 or undefined. We'll return 0.
        // Check if query point *is* the single point location
        if (mbgl::util::haversineDist(geometry_[0], queryPoint) > EPSILON) {
            // If query point is different, maybe success should be false? Depends on requirements.
            // Let's keep it true but maybe add a warning/note.
            Log::Debug(Event::Route, "Warning: Route has zero total length. Closest point set to route start.");
        }
        return 0.0;
    }

    double minDist = std::numeric_limits<double>::max();
    double bestSegmentFraction = 0.0;
    size_t bestSegmentIndex = 0;
    Vector3D vQuery = latLonToCartesian(queryPoint);

    for (size_t i = 0; i < geometry_.size() - 1; ++i) {
        const mbgl::Point<double>& p1 = geometry_[i];     // Start point of the segment
        const mbgl::Point<double>& p2 = geometry_[i + 1]; // End point of the segment
        Vector3D v1 = latLonToCartesian(p1);
        Vector3D v2 = latLonToCartesian(p2);

        // --- Project vQuery onto the 3D line segment (chord) v1 -> v2 ---
        Vector3D segmentVec = vecSubtract(v2, v1);
        Vector3D queryVec = vecSubtract(vQuery, v1); // Vector from v1 to vQuery

        double segmentLenSq = vecMagnitudeSq(segmentVec);
        double t = 0.0; // Parameter along the chord (0=v1, 1=v2)

        if (segmentLenSq > EPSILON) {
            // Project queryVec onto segmentVec
            t = vecDot(queryVec, segmentVec) / segmentLenSq;
            t = std::clamp(t, 0.0, 1.0); // Clamp to the segment bounds [0, 1]
        } else {
            // Segment chord has zero length (v1 and v2 are same point)
            // The closest point on the segment is v1 (or v2).
            t = 0.0;
        }

        // --- Find the point on the great circle arc corresponding to fraction t ---
        // We need the point on the *arc* to calculate the true distance.
        // We use the SLERP (interpolation) to find this point, even
        // though 't' came from projection onto the chord. This is a common approximation.
        Point<double> closestPointOnArc;
        double omega = 0.0;
        double dot_prod = vecDot(v1, v2);
        dot_prod = std::clamp(dot_prod, -1.0, 1.0); // Clamp for acos safety
        omega = std::acos(dot_prod);                // Angle of the arc segment

        if (std::abs(omega) < EPSILON || std::abs(std::sin(omega)) < EPSILON) {
            // Arc angle is negligible (points identical) or antipodal.
            // If identical (omega ~ 0), the point is just p1 (or p2).
            // If antipodal (omega ~ PI), the path is ambiguous, but LERP is okay fallback.
            // Use simple linear interpolation on original lat/lon for safety here.
            closestPointOnArc.y = p1.y + t * (p2.y - p1.y);
            closestPointOnArc.x = p1.x + t * (p2.x - p1.x);
        } else {
            // Use SLERP formula to find the point on the arc at fraction t
            double sin_omega = std::sin(omega);
            double a = std::sin((1.0 - t) * omega) / sin_omega;
            double b = std::sin(t * omega) / sin_omega;
            Vector3D v_result = vecAdd(vecScale(v1, a), vecScale(v2, b));
            closestPointOnArc = cartesianToLatLon(v_result);
        }

        // --- Calculate distance from queryPoint to this point on the arc ---
        double dist = mbgl::util::haversineDist(queryPoint, closestPointOnArc);

        if (dist < minDist) {
            minDist = dist;
            bestSegmentIndex = i;
            bestSegmentFraction = t; // Store the fraction along the chord/arc
        }
    }

    // --- Calculate final percentage (same as LERP version) ---
    double distance_along_route = cumulativeIntervalDistances_[bestSegmentIndex] +
                                  (bestSegmentFraction * intervalLengths_[bestSegmentIndex]);

    // Clamp final distance just in case of minor over/undershoot from projection
    distance_along_route = std::clamp(distance_along_route, 0.0, totalLength_);

    return distance_along_route / totalLength_;
}

uint32_t Route::getNumRouteSegments() const {
    return static_cast<uint32_t>(segments_.size());
}

std::vector<double> Route::getRouteSegmentDistances() const {
    return intervalLengths_;
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
