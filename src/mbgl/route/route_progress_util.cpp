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

double RouteProgressUtil::haversineDistance_boost(const GeoPoint& p1, const GeoPoint& p2) {
    double lat1Rad = degreesToRadians_boost(p1.latitude);
    double lon1Rad = degreesToRadians_boost(p1.longitude);
    double lat2Rad = degreesToRadians_boost(p2.latitude);
    double lon2Rad = degreesToRadians_boost(p2.longitude);
    double dLat = lat2Rad - lat1Rad;
    double dLon = lon2Rad - lon1Rad;
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1Rad) * std::cos(lat2Rad) * std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return EARTH_RADIUS_METERS_BOOST * c;
}

RouteProperties::RouteProperties(const std::vector<GeoPoint>& route) {
    if (route.empty()) return;

    cumulative_distances.push_back(0.0);
    std::vector<RTreeValue> rtree_values; // For bulk loading R-tree

    if (route.size() > 1) {
        for (size_t i = 0; i < route.size() - 1; ++i) {
            const GeoPoint& p1 = route[i];
            const GeoPoint& p2 = route[i + 1];

            // Calculate Haversine distance for segment length
            double dist = RouteProgressUtil::haversineDistance_boost(p1, p2);
            segment_lengths.push_back(dist);
            total_distance += dist;
            cumulative_distances.push_back(total_distance);

            // Create MBR for the R-tree
            // BoostPoint uses (x,y) -> (longitude, latitude)
            BoostPoint bp1(p1.longitude, p1.latitude);
            BoostPoint bp2(p2.longitude, p2.latitude);
            BoostBox segment_mbr;
            bg::envelope(bp1, segment_mbr); // Start MBR with first point
            bg::expand(segment_mbr, bp2);   // Expand MBR to include second point

            rtree_values.push_back(std::make_pair(segment_mbr, i)); // Store MBR and original segment index
        }
    }
    // Bulk load the R-tree
    if (!rtree_values.empty()) {
        // The constructor chosen here performs bulk loading
        rtree = RTree(rtree_values.begin(), rtree_values.end());
    }
}

// Helper function to project a GeoPoint onto a segment using LERP logic
ProjectionInfoBoost project_onto_segment_lerp_boost(const GeoPoint& segP1,
                                                    const GeoPoint& segP2,
                                                    const GeoPoint& queryPoint) {
    ProjectionInfoBoost info;

    double seg_dx_lon = segP2.longitude - segP1.longitude;
    double seg_dy_lat = segP2.latitude - segP1.latitude;

    double segment_len_sq_cartesian_approx = seg_dx_lon * seg_dx_lon + seg_dy_lat * seg_dy_lat;
    double t = 0.0;

    if (segment_len_sq_cartesian_approx > EPSILON_BOOST) {
        double qp1_dx_lon = queryPoint.longitude - segP1.longitude;
        double qp1_dy_lat = queryPoint.latitude - segP1.latitude;
        t = (qp1_dx_lon * seg_dx_lon + qp1_dy_lat * seg_dy_lat) / segment_len_sq_cartesian_approx;
        t = std::clamp(t, 0.0, 1.0);
    }

    info.closest_point_on_segment.latitude = segP1.latitude + t * seg_dy_lat;
    info.closest_point_on_segment.longitude = segP1.longitude + t * seg_dx_lon;
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
GeoPoint RouteProgressUtil::getPointAlongRoute_Boost(const std::vector<GeoPoint>& route,
                                                     double percentage,
                                                     const RouteProperties& props) {
    if (route.empty()) {
        throw std::invalid_argument("Route cannot be empty.");
    }
    if (route.size() == 1) return route[0];

    percentage = std::clamp(percentage, 0.0, 1.0);

    if (percentage == 0.0) return route.front();
    if (percentage == 1.0) return route.back();
    if (props.total_distance <= EPSILON_BOOST) return route.front();

    double target_distance = props.total_distance * percentage;
    double cumulative_dist_so_far = 0.0;

    if (props.segment_lengths.empty() && route.size() > 1) {
        // This case implies a route of 2+ points but 0 total length, or props not init correctly.
        return route.front();
    }

    for (size_t i = 0; i < props.segment_lengths.size(); ++i) {
        double current_segment_length = props.segment_lengths[i];
        // Ensure i+1 is a valid index for route
        if (i + 1 >= route.size()) {
            // Should not happen if segment_lengths is consistent with route size
            return route.back();
        }

        if (cumulative_dist_so_far + current_segment_length >= target_distance - EPSILON_BOOST) {
            const GeoPoint& p1 = route[i];
            const GeoPoint& p2 = route[i + 1];
            double distance_needed_in_segment = target_distance - cumulative_dist_so_far;
            double segment_fraction = 0.0;
            if (current_segment_length > EPSILON_BOOST) {
                segment_fraction = distance_needed_in_segment / current_segment_length;
            }
            segment_fraction = std::clamp(segment_fraction, 0.0, 1.0);

            // LERP for point on segment
            GeoPoint result_point;
            result_point.latitude = p1.latitude + (p2.latitude - p1.latitude) * segment_fraction;
            result_point.longitude = p1.longitude + (p2.longitude - p1.longitude) * segment_fraction;
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
double RouteProgressUtil::getPercentageAlongRoute_BoostRTree(const std::vector<GeoPoint>& route,
                                                             const GeoPoint& queryPoint,
                                                             const RouteProperties& props) {
    if (route.empty()) {
        // Or throw std::invalid_argument("Route cannot be empty.");
        return 0.0;
    }
    if (route.size() == 1) {
        return 0.0; // Closest point is the only point.
    }
    if (props.total_distance <= EPSILON_BOOST) {
        return 0.0; // Route has zero length.
    }

    // --- Use R-tree to find candidate segments ---
    std::vector<RTreeValue> candidate_rtree_values;
    BoostPoint query_boost_point(queryPoint.longitude, queryPoint.latitude);

    // Query for K nearest segments. K needs to be chosen.
    const unsigned int K_NEAREST_CANDIDATES = 5;
    props.rtree.query(bgi::nearest(query_boost_point, K_NEAREST_CANDIDATES),
                      std::back_inserter(candidate_rtree_values));

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

            const GeoPoint& p1 = route[segment_idx];
            const GeoPoint& p2 = route[segment_idx + 1];
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
    // Ensure best_segment_index is valid for props.cumulative_distances and props.segment_lengths
    if (static_cast<size_t>(best_segment_index) >=
            props.cumulative_distances.size() ||                     // cumulative_distances has N+1 elements
        (props.segment_lengths.empty() && best_segment_index > 0) || // No segments but index > 0
        (!props.segment_lengths.empty() && static_cast<size_t>(best_segment_index) >= props.segment_lengths.size())) {
        if (!route.empty() && props.total_distance > EPSILON_BOOST) {
            std::cerr << "Warning: Invalid best_segment_index (" << best_segment_index << ") for segment_lengths size ("
                      << props.segment_lengths.size() << ") or cumulative_distances size ("
                      << props.cumulative_distances.size() << "). Defaulting percentage." << std::endl;
        }
        return 0.0;
    }

    double distance_along_route = props.cumulative_distances[best_segment_index] +
                                  (best_projection.fraction_on_segment * props.segment_lengths[best_segment_index]);

    distance_along_route = std::clamp(distance_along_route, 0.0, props.total_distance);
    return distance_along_route / props.total_distance;
}

} // namespace route
} // namespace mbgl
