#pragma once

// Boost.Geometry includes
#include <mbgl/route/rtree_def.hpp>

// #include <boost/geometry.hpp>
// #include <boost/geometry/geometries/point_xy.hpp>
// #include <boost/geometry/geometries/box.hpp>
// #include <boost/geometry/index/rtree.hpp>

namespace mbgl {
namespace route {

struct ProjectionInfoBoost {
    double distance_to_query_point;
    double fraction_on_segment; // Theroute.h 't' parameter (0.0 to 1.0)
    Point<double> closest_point_on_segment;
};

// namespace bg = boost::geometry;
// namespace bgi = boost::geometry::index;
//
// typedef bg::model::d2::point_xy<double> BoostPoint;      // For (longitude, latitude)
// typedef bg::model::box<BoostPoint> BoostBox;             // MBR of a segment
// typedef std::pair<BoostBox, size_t> RTreeValue;          // Store MBR and original segment index
// typedef bgi::rtree<RTreeValue, bgi::rstar<16, 4>> RTree; // R*-tree variant, 16 max entries/node

// --- Constants & Helper functions (Haversine, Point struct etc.) ---
constexpr double PI_BOOST = 3.14159265358979323846;
constexpr double EARTH_RADIUS_METERS_BOOST = 6371000.0;
constexpr double EPSILON_BOOST = 1e-10;

// --- Precomputed Route Properties Structure ---

// struct RouteProperties {
//     std::vector<double> segment_lengths;
//     std::vector<double> cumulative_distances;
//     double total_distance = 0.0;
//     RTree rtree; // R-tree built from the route segments
//
//     RouteProperties(const std::vector<Point<double>>& route);
// };

class RouteProgressUtil {
private:
public:
    static Point<double> getPointAlongRoute_Boost(const std::vector<Point<double>>& route,
                                                  double percentage,
                                                  const std::vector<double>& intervalLengths,
                                                  double totalDistance);
    static double getPercentageAlongRoute_BoostRTree(const std::vector<Point<double>>& route,
                                                     const Point<double>& queryPoint,
                                                     const std::vector<double>& interval_lengths,
                                                     const std::vector<double>& cumulative_distances,
                                                     double total_distance,
                                                     std::shared_ptr<RTree> rtree);
    static double haversineDistance_boost(const Point<double>& p1, const Point<double>& p2);
    static double degreesToRadians_boost(double degrees);
};

} // namespace route
} // namespace mbgl
