#include <mbgl/route/route_utils.hpp>

namespace mbgl {
namespace route {

bool RouteUtils::isPointBetween(const mbgl::Point<double>& A, const mbgl::Point<double>& B, const mbgl::Point<double>& C) {
    // Check if the points are collinear (on the same line)
    double crossProduct = (C.y - B.y) * (A.x - B.x) - (C.x - B.x) * (A.y - B.y);

    if (std::abs(crossProduct) > 1e-9) { // Using a small epsilon for floating-point comparison
        return false; // Not collinear, so A cannot be between B and C
    }

    // Check if A is within the bounding box of B and C.  This is a simplified version.
    // A more robust check would involve projecting A onto the line BC and checking
    // if the projection lies within the segment BC.  The bounding box check is sufficient
    // for many common cases and is less computationally expensive.

    double minX = std::min(B.x, C.x);
    double maxX = std::max(B.x, C.x);
    double minY = std::min(B.y, C.y);
    double maxY = std::max(B.y, C.y);

    return (A.x >= minX && A.x <= maxX && A.y >= minY && A.y <= maxY);


    // More robust check (projection method - commented out for brevity, but recommended):
    /*
    double dotProduct = (A.x - B.x) * (C.x - B.x) + (A.y - B.y) * (C.y - B.y);
    double squaredLengthBC = (C.x - B.x) * (C.x - B.x) + (C.y - B.y) * (C.y - B.y);

    if (squaredLengthBC == 0) { // B and C are the same point
        return (A.x == B.x && A.y == B.y); // A must be equal to B (and C)
    }

    double t = dotProduct / squaredLengthBC;

    return (t >= 0 && t <= 1);
    */
}

}
}