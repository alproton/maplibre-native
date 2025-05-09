#include <mbgl/route/route_progress_util.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm> // For std::min, std::max, std::clamp
#include <limits>    // For std::numeric_limits
#include <stdexcept> // For exceptions
#include <iomanip>   // For setting precision

namespace mbgl {
namespace route {

double RouteProgressUtil::degreesToRadians_boost(double degrees) {
    return degrees * PI_BOOST / 180.0;
}

double RouteProgressUtil::haversineDistance_boost(const Point<double>& p1, const Point<double>& p2) {
    double lat1Rad = degreesToRadians_boost(p1.y);
    double lon1Rad = degreesToRadians_boost(p1.x);
    double lat2Rad = degreesToRadians_boost(p2.y);
    double lon2Rad = degreesToRadians_boost(p2.x);
    double dLat = lat2Rad - lat1Rad;
    double dLon = lon2Rad - lon1Rad;
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1Rad) * std::cos(lat2Rad) * std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return EARTH_RADIUS_METERS_BOOST * c;
}

// RouteProperties::RouteProperties(const std::vector<Point<double>>& route) {
//     if (route.empty()) return;
//
//     cumulative_distances.push_back(0.0);
//     std::vector<RTreeValue> rtree_values; // For bulk loading R-tree
//
//     if (route.size() > 1) {
//         for (size_t i = 0; i < route.size() - 1; ++i) {
//             const Point<double>& p1 = route[i];
//             const Point<double>& p2 = route[i + 1];
//
//             // Calculate Haversine distance for segment length
//             double dist = RouteProgressUtil::haversineDistance_boost(p1, p2);
//             segment_lengths.push_back(dist);
//             total_distance += dist;
//             cumulative_distances.push_back(total_distance);
//
//             // Create MBR for the R-tree
//             // BoostPoint uses (x,y) -> (longitude, latitude)
//             BoostPoint bp1(p1.x, p1.y);
//             BoostPoint bp2(p2.x, p2.y);
//             BoostBox segment_mbr;
//             bg::envelope(bp1, segment_mbr); // Start MBR with first point
//             bg::expand(segment_mbr, bp2);   // Expand MBR to include second point
//
//             rtree_values.push_back(std::make_pair(segment_mbr, i)); // Store MBR and original segment index
//         }
//     }
//     // Bulk load the R-tree
//     if (!rtree_values.empty()) {
//         // The constructor chosen here performs bulk loading
//         rtree = RTree(rtree_values.begin(), rtree_values.end());
//     }
// }

// Helper function to project a GeoPoint onto a segment using LERP logic
ProjectionInfoBoost project_onto_segment_lerp_boost(const Point<double>& segP1,
                                                    const Point<double>& segP2,
                                                    const Point<double>& queryPoint) {
    ProjectionInfoBoost info;

    double seg_dx_lon = segP2.x - segP1.x;
    double seg_dy_lat = segP2.y - segP1.y;

    double segment_len_sq_cartesian_approx = seg_dx_lon * seg_dx_lon + seg_dy_lat * seg_dy_lat;
    double t = 0.0;

    if (segment_len_sq_cartesian_approx > EPSILON_BOOST) {
        double qp1_dx_lon = queryPoint.x - segP1.x;
        double qp1_dy_lat = queryPoint.y - segP1.y;
        t = (qp1_dx_lon * seg_dx_lon + qp1_dy_lat * seg_dy_lat) / segment_len_sq_cartesian_approx;
        t = std::clamp(t, 0.0, 1.0);
    }

    info.closest_point_on_segment.y = segP1.y + t * seg_dy_lat;
    info.closest_point_on_segment.x = segP1.x + t * seg_dx_lon;
    info.fraction_on_segment = t;
    info.distance_to_query_point = RouteProgressUtil::haversineDistance_boost(queryPoint,
                                                                              info.closest_point_on_segment);

    return info;
}

/**
 * @brief Returns a GeoPoint along the route at a given percentage of its total length.
 * This function does NOT use the R-Tree.
 *
 * @param route The vector of GeoPoints representing the route.
 * @param percentage A value between 0.0 and 1.0.
 * @param props Precomputed route properties.
 * @return The GeoPoint at the specified percentage along the route.
 */
Point<double> RouteProgressUtil::getPointAlongRoute_Boost(const std::vector<Point<double>>& route,
                                                          double percentage,
                                                          const std::vector<double>& interval_lengths,
                                                          double total_distance) {
    if (route.empty()) {
        throw std::invalid_argument("Route cannot be empty.");
    }
    if (route.size() == 1) return route[0];

    percentage = std::clamp(percentage, 0.0, 1.0);

    if (percentage == 0.0) return route.front();
    if (percentage == 1.0) return route.back();
    if (total_distance <= EPSILON_BOOST) return route.front();

    double target_distance = total_distance * percentage;
    double cumulative_dist_so_far = 0.0;

    if (interval_lengths.empty() && route.size() > 1) {
        // This case implies a route of 2+ points but 0 total length, or props not init correctly.
        return route.front();
    }

    for (size_t i = 0; i < interval_lengths.size(); ++i) {
        double current_segment_length = interval_lengths[i];
        // Ensure i+1 is a valid index for route
        if (i + 1 >= route.size()) {
            // Should not happen if segment_lengths is consistent with route size
            return route.back();
        }

        if (cumulative_dist_so_far + current_segment_length >= target_distance - EPSILON_BOOST) {
            const Point<double>& p1 = route[i];
            const Point<double>& p2 = route[i + 1];
            double distance_needed_in_segment = target_distance - cumulative_dist_so_far;
            double segment_fraction = 0.0;
            if (current_segment_length > EPSILON_BOOST) {
                segment_fraction = distance_needed_in_segment / current_segment_length;
            }
            segment_fraction = std::clamp(segment_fraction, 0.0, 1.0);

            // LERP for point on segment
            Point<double> result_point;
            result_point.y = p1.y + (p2.y - p1.y) * segment_fraction;
            result_point.x = p1.x + (p2.x - p1.x) * segment_fraction;
            return result_point;
        }
        cumulative_dist_so_far += current_segment_length;
    }
    return route.back(); // Fallback for percentage = 1.0 or float issues
}

/**
 * @brief Finds the percentage along a route corresponding to the closest point
 * on the route to a given query point, using Boost.Geometry R-tree for optimization.
 *
 * @param route The vector of GeoPoints representing the route.
 * @param queryPoint The GeoPoint to find the closest match for on the route.
 * @param props Precomputed route properties including the R-tree.
 * @return The percentage (0.0 to 1.0) along the total route length.
 */
double RouteProgressUtil::getPercentageAlongRoute_BoostRTree(const std::vector<Point<double>>& route,
                                                             const Point<double>& queryPoint,
                                                             const std::vector<double>& interval_lengths,
                                                             const std::vector<double>& cumulative_distances,
                                                             double total_distance,
                                                             std::shared_ptr<RTree> rtree) {
    if (route.empty()) {
        // Or throw std::invalid_argument("Route cannot be empty.");
        return 0.0;
    }
    if (route.size() == 1) {
        return 0.0; // Closest point is the only point.
    }
    if (total_distance <= EPSILON_BOOST) {
        return 0.0; // Route has zero length.
    }

    // --- Use R-tree to find candidate segments ---
    std::vector<RTreeValue> candidate_rtree_values;
    BoostPoint query_boost_point(queryPoint.x, queryPoint.x);

    // Query for K nearest segments. K needs to be chosen.
    const unsigned int K_NEAREST_CANDIDATES = 5;
    rtree->query(bgi::nearest(query_boost_point, K_NEAREST_CANDIDATES), std::back_inserter(candidate_rtree_values));

    ProjectionInfoBoost best_projection;
    best_projection.distance_to_query_point = std::numeric_limits<double>::max();
    int best_segment_index = 0;

    if (candidate_rtree_values.empty() && route.size() > 1) {
        // Fallback logic: if R-tree query returns nothing (unlikely with bgi::nearest if K > 0 and tree not empty)
        // For robustness, prepare to check all segments if needed.
        for (size_t i = 0; i < route.size() - 1; ++i) {
            candidate_rtree_values.push_back(std::make_pair(BoostBox(BoostPoint(0, 0), BoostPoint(0, 0)), i));
        }
        if (candidate_rtree_values.empty() &&
            (route.size() - 1 > 0)) { // Should be at least one segment if route.size() > 1
            candidate_rtree_values.push_back(std::make_pair(BoostBox(BoostPoint(0, 0), BoostPoint(0, 0)), 0));
        }
    }

    bool found_any_candidate = false;
    if (!candidate_rtree_values.empty()) {
        for (const auto& rtree_val : candidate_rtree_values) {
            size_t segment_idx = rtree_val.second; // Get original segment index

            if (segment_idx + 1 >= route.size()) continue;

            const Point<double>& p1 = route[segment_idx];
            const Point<double>& p2 = route[segment_idx + 1];
            ProjectionInfoBoost current_projection = project_onto_segment_lerp_boost(p1, p2, queryPoint);

            if (current_projection.distance_to_query_point < best_projection.distance_to_query_point) {
                best_projection = current_projection;
                best_segment_index = static_cast<int>(segment_idx);
            }
            found_any_candidate = true;
        }
    }

    if (!found_any_candidate && !route.empty()) {
        // If after all, no candidate was processed (e.g., route had 1 point, or some other edge case).
        best_segment_index = 0; // Default to first segment
        best_projection = project_onto_segment_lerp_boost(route[0], route.size() > 1 ? route[1] : route[0], queryPoint);
    }

    // --- Calculate final percentage ---
    // Ensure best_segment_index is valid for cumulative_distances and interval_lengths
    if (static_cast<size_t>(best_segment_index) >=
            cumulative_distances.size() ||                      // cumulative_distances has N+1 elements
        (interval_lengths.empty() && best_segment_index > 0) || // No segments but index > 0
        (!interval_lengths.empty() && static_cast<size_t>(best_segment_index) >= interval_lengths.size())) {
        if (!route.empty() && total_distance > EPSILON_BOOST) {
            std::cerr << "Warning: Invalid best_segment_index (" << best_segment_index << ") for segment_lengths size ("
                      << interval_lengths.size() << ") or cumulative_distances size (" << cumulative_distances.size()
                      << "). Defaulting percentage." << std::endl;
        }
        return 0.0;
    }

    double distance_along_route = cumulative_distances[best_segment_index] +
                                  (best_projection.fraction_on_segment * interval_lengths[best_segment_index]);

    distance_along_route = std::clamp(distance_along_route, 0.0, total_distance);
    return distance_along_route / total_distance;
}

} // namespace route
} // namespace mbgl
