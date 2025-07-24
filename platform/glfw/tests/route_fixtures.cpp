#include "route_fixtures.hpp"
#include <mbgl/util/math.hpp>

namespace route_fixtures {
mbgl::Point<double> RouteData::getPoint(double percent) const {
    if (points.empty()) {
        return {0.0, 0.0};
    }

    if (percent <= 0.0) {
        return points.front();
    }

    if (percent >= 1.0) {
        return points.back();
    }

    double totalLength = 0.0;
    std::vector<double> segmentLengths(points.size() - 1);

    // Calculate the length of each segment and the total length of the polyline
    for (size_t i = 0; i < points.size() - 1; ++i) {
        double dist = mbgl::util::dist<double>(points[i], points[i + 1]);
        segmentLengths[i] = dist;
        totalLength += segmentLengths[i];
    }

    double targetLength = percent * totalLength;
    double accumulatedLength = 0.0;

    // Find the segment where the target length falls
    for (size_t i = 0; i < segmentLengths.size() - 1; ++i) {
        if (accumulatedLength + segmentLengths[i] >= targetLength) {
            double segmentPercent = (targetLength - accumulatedLength) / segmentLengths[i];
            double x = points[i].x + segmentPercent * (points[i + 1].x - points[i].x);
            double y = points[i].y + segmentPercent * (points[i + 1].y - points[i].y);
            return {x, y};
        }
        accumulatedLength += segmentLengths[i];
    }

    return points.back();
}

std::pair<uint32_t, double> RouteData::getIntervalFraction(double percent) const {
    const double EPSILON = 1e-8;
    const auto& getClosestInterval = [&](const mbgl::Point<double>& queryPoint) -> std::pair<uint32_t, double> {
        uint32_t bestIntervalIndex = INVALID_UINT;
        double bestIntervalFraction = -1.0;
        double minDistanceSq = std::numeric_limits<double>::max();

        for (size_t i = 0; i < points.size(); ++i) {
            const mbgl::Point<double>& p1 = points[i];     // Start point of the segment
            const mbgl::Point<double>& p2 = points[i + 1]; // End point of the segment

            // --- Project queryPoint onto the 2D line segment (p1, p2) ---
            // Treat Lat/Lon as simple 2D coordinates for projection (approximation)
            double segmentDX = p2.x - p1.x; // longitude
            double segmentDY = p2.y - p1.y; // lattitude

            double segmentLenSq = segmentDX * segmentDX + segmentDY * segmentDY;

            double t = 0.0; // Parameter along the line segment (0=p1, 1=p2)
            mbgl::Point<double> closestPointOnSegment = p1;

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

    const mbgl::Point<double>& queryPoint = getPoint(percent);
    return getClosestInterval(queryPoint);
}

} // namespace route_fixtures
