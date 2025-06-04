#pragma once

#include <mbgl/route/id_types.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/route/id_pool.hpp>
#include <mbgl/route/route.hpp>
#include <mbgl/route/route_enums.hpp>
#include <unordered_map>
#include <unordered_set>
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
    double avgRouteCreationInterval = 0.0;
    double avgRouteSegmentCreationInterval = 0.0;
    long long maxRouteVanishingElapsedMillis = 0.0;
    long long minRouteVanishingElapsedMillis = 0.0;
    double avgRouteVanishingElapsedMillis = 0.0;
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
    const std::string getStats();
    bool hasStyle() const;
    /**
     * This method is currently used when we need to create multiple routes at once when de-serializing from a snapshot
     * captured.
     * @param routeID the specified starting route ID
     * @param numRoutes the number of route containers to create
     * @return the starting routeID, which typically should be same as input routeID, invalid route ID is returned
     * if consecutive range of routes cannot be created.
     */
    RouteID routePreCreate(const RouteID& routeID, uint32_t numRoutes);

    /***
     * Sets the route data on a pre-created routeID that was allocated in batch using routePreCreate.
     * @param routeID the specified routeID.
     * @param geometry the specified geometry of the route.
     * @param ropts the specified route options.
     */
    bool routeSet(const RouteID& routeID, const LineString<double>& geometry, const RouteOptions& ropts);

    RouteID routeCreate(const LineString<double>& geometry, const RouteOptions& ropts);
    bool routeSegmentCreate(const RouteID&, const RouteSegmentOptions&);
    bool routeSetProgressPercent(const RouteID&, double progress);
    double routeSetProgressPoint(const RouteID&, const Point<double>& progressPoint, const Precision& precision);
    void setUseRouteSegmentIndexFractions(bool useFractions);
    double routeSetProgressInMeters(const RouteID& routeID, double progresInMeters);
    Point<double> getPoint(const RouteID& routeID,
                           double percent,
                           const Precision& precision,
                           double* bearing = nullptr) const;
    std::optional<LineString<double>> routeGetGeometry(const RouteID& routeID) const;
    void routeClearSegments(const RouteID&);
    bool routeDispose(const RouteID&);
    bool setVanishingRouteID(const RouteID& routeID);
    RouteID getVanishingRouteID() const;
    std::vector<RouteID> getAllRoutes() const;
    std::string getActiveRouteLayerName(const RouteID& routeID) const;
    std::string getBaseRouteLayerName(const RouteID& routeID) const;
    std::string getActiveGeoJSONsourceName(const RouteID& routeID) const;
    std::string getBaseGeoJSONsourceName(const RouteID& routeID) const;
    std::string captureSnapshot() const;
    bool loadCapture(const std::string& capture);
    bool captureScrubRoute(double scrubValue, Point<double>* optPointOut = nullptr, double* optBearingOut = nullptr);
    int getTopMost(const std::vector<RouteID>& routeList) const;
    void captureNavStops(bool onOff);
    bool isCaptureNavStopsEnabled() const;
    bool hasRoutes() const;
    void finalize();

    ~RouteManager();

private:
    static const std::string CASING_ROUTE_LAYER;
    static const std::string ACTIVE_ROUTE_LAYER;
    static const std::string GEOJSON_CASING_ROUTE_SOURCE_ID;
    static const std::string GEOJSON_ACTIVE_ROUTE_SOURCE_ID;

    enum class DirtyType {
        dtRouteSegments,
        dtRouteProgress,
        dtRouteGeometry,
        // TODO: may be route puck position
    };
    std::string dirtyTypeToString(const DirtyType& dt) const;

    std::unordered_map<DirtyType, std::unordered_set<RouteID, IDHasher<RouteID>>> dirtyRouteMap_;
    std::vector<std::string> apiCalls_;

    RouteMgrStats stats_;
    gfx::IDpool routeIDpool_ = gfx::IDpool(100);

    void finalizeRoute(const RouteID& routeID, const DirtyType& dt);
    void validateAddToDirtyBin(const RouteID& routeID, const DirtyType& dirtyBin);
    // TODO: change this to weak reference
    style::Style* style_ = nullptr;
    std::unordered_map<RouteID, Route, IDHasher<RouteID>> routeMap_;
    RouteID vanishingRouteID_;
    long long totalVanishingRouteElapsedMillis = 0;
    long long numVanisingRouteInvocations = 0;
    bool captureNavStops_ = false;
    bool useRouteSegmentIndexFractions_ = false;
};
}; // namespace route

} // namespace mbgl
