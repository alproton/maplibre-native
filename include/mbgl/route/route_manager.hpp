#pragma once

#include <mbgl/route/id_types.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/route/id_pool.hpp>
#include <mbgl/route/route.hpp>
#include <unordered_map>
#include <string>

namespace mbgl {

namespace style {
class Style;
} // namespace style

namespace route {


struct RouteMgrStats {
    uint32_t numFinalizedInvoked = 0;
    uint32_t numRoutes = 0;
    uint32_t numRouteSegments = 0;
    std::string finalizeMillis;
    bool inconsistentAPIusage = false;
};



/***
 * A route manager manages construction, disposal and updating of one or more routes. It is the API facade and is 1:1
 *with a map view. You can create and mutate multiple routes as many times and after you're done with mutating routes,
 *the client code needs to call finalize() on the route manager for it to create needed resources underneath the hood
 *for rendering.
 **/
class RouteManager final {
public:
    RouteManager();
    void setStyle(style::Style&);
    const std::string getStats() const;
    void clearStats();
    bool hasStyle() const;
    RouteID routeCreate(const LineString<double>& geometry, const RouteOptions& ropts);
    void routeSegmentCreate(const RouteID&, const RouteSegmentOptions&);
    bool routeSetProgress(const RouteID&, const double progress);
    void routeClearSegments(const RouteID&);
    bool routeDispose(const RouteID&);
    std::string getActiveRouteLayerName(const RouteID& routeID) const;
    std::string getBaseRouteLayerName(const RouteID& routeID) const;
    bool hasRoutes() const;
    void finalize();

    ~RouteManager();

private:
    static const std::string BASE_ROUTE_LAYER;
    static const std::string ACTIVE_ROUTE_LAYER;
    static const std::string GEOJSON_BASE_ROUTE_SOURCE_ID;
    static const std::string GEOJSON_ACTIVE_ROUTE_SOURCE_ID;
    static std::stringstream ss_;

    enum class DirtyType {
        dtRouteSegments,
        dtRouteProgress,
        dtRouteGeometry,
        //TODO: may be route puck position
    };

    std::unordered_map<DirtyType, std::vector<RouteID>> dirtyRouteMap_;

    RouteMgrStats stats_;
    uint32_t eventID_ = 0;
    gfx::IDpool routeIDpool_ = gfx::IDpool(100);
    std::string getActiveGeoJSONsourceName(const RouteID& routeID) const;
    std::string getBaseGeoJSONsourceName(const RouteID& routeID) const;
    void finalizeRoute(const RouteID& routeID, const DirtyType& dt);

    // TODO: change this to weak reference
    style::Style* style_ = nullptr;
    std::unordered_map<RouteID, Route, IDHasher<RouteID>> routeMap_;
};
}; // namespace route

} // namespace mbgl
