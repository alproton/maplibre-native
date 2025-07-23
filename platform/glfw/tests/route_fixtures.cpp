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

} // namespace route_fixtures
