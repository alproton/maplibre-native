#pragma once

// Boost.Geometry includes
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry.hpp>

namespace mbgl {
namespace route {

struct GeoPoint {
    double latitude;
    double longitude;
};

struct ProjectionInfoBoost {
    double distance_to_query_point;
    double fraction_on_segment; // The 't' parameter (0.0 to 1.0)
    GeoPoint closest_point_on_segment;
};

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::d2::point_xy<double> BoostPoint;      // For (longitude, latitude)
typedef bg::model::box<BoostPoint> BoostBox;             // MBR of a segment
typedef std::pair<BoostBox, size_t> RTreeValue;          // Store MBR and original segment index
typedef bgi::rtree<RTreeValue, bgi::rstar<16, 4>> RTree; // R*-tree variant, 16 max entries/node

// --- Constants & Helper functions (Haversine, Point struct etc.) ---
constexpr double PI_BOOST = 3.14159265358979323846;
constexpr double EARTH_RADIUS_METERS_BOOST = 6371000.0;
constexpr double EPSILON_BOOST = 1e-10;

// --- Precomputed Route Properties Structure ---

struct RouteProperties {
    std::vector<double> segment_lengths;
    std::vector<double> cumulative_distances;
    double total_distance = 0.0;
    RTree rtree; // R-tree built from the route segments

    RouteProperties(const std::vector<GeoPoint>& route);
};

class RouteProgressUtil {
private:
public:
    static GeoPoint getPointAlongRoute_Boost(const std::vector<GeoPoint>& route,
                                             double percentage,
                                             const RouteProperties& props);
    static double getPercentageAlongRoute_BoostRTree(const std::vector<GeoPoint>& route,
                                                     const GeoPoint& queryPoint,
                                                     const RouteProperties& props);
    static double haversineDistance_boost(const GeoPoint& p1, const GeoPoint& p2);
    static double degreesToRadians_boost(double degrees);
};

} // namespace route
} // namespace mbgl
