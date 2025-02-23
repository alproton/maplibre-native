#pragma once

#include <mbgl/route/id_types.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/route/id_pool.hpp>
#include <unordered_map>
#include <string>

namespace mbgl {


namespace style {
class Style;
} // namespace style

namespace route {

class Route;

struct RouteCommonOptions {
    Color outerColor = Color(1, 1, 1, 1);
    Color innerColor = Color(0, 0, 1, 1);
    float outerWidth = 10;
    float innerWidth = 6;
};

struct RouteMgrStats {
    uint32_t numFinalizedInvoked = 0;
    uint32_t numRoutes = 0;
    uint32_t numRouteSegments = 0;
};


/***
 * A route is a road between two locations. There can be multiple routes in the case of multiple stops.
 * Each route can have route segments. Routes segments can be used for traffic data.
 * A route must have a base route segment that contains all the vertices of the polyline that makes it.
 */
class RouteManager final {
public:
    // static const std::string BASE_ROUTE_SEGMENT_STR;

    RouteManager();
  void setStyle(style::Style&);
  static void appendStats(const std::string& str);
  static const std::string getStats();
  static void clearStats();
  bool hasStyle() const;
  void setLayerBefore(const std::string layerBefore);
  void setRouteCommonOptions(const RouteCommonOptions& ropts);
  RouteID routeCreate(const LineString<double>& geometry);
  void routeSegmentCreate(const RouteID&, const RouteSegmentOptions&);
    bool routeSetProgress(const RouteID&, const double progress);
    void routeClearSegments(const RouteID&);
  bool routeDispose(const RouteID&);
    bool hasRoutes() const;
  void finalize();

  ~RouteManager();
private:
    static const std::string BASE_ROUTE_LAYER;
    static const std::string ACTIVE_ROUTE_LAYER;
    static const std::string GEOJSON_BASE_ROUTE_SOURCE_ID;
    static const std::string GEOJSON_ACTIVE_ROUTE_SOURCE_ID;
    static std::stringstream ss_;

    RouteMgrStats stats_;
  gfx::IDpool routeIDpool_ = gfx::IDpool(100);
  //TODO: change this to weak reference
  std::string getActiveRouteLayerName(const RouteID& routeID) const;
  std::string getBaseRouteLayerName(const RouteID& routeID) const;
  std::string getActiveGeoJSONsourceName(const RouteID& routeID) const;
  std::string getBaseGeoJSONsourceName(const RouteID& routeID) const;

  style::Style* style_ = nullptr;
  std::unordered_map<RouteID, Route, IDHasher<RouteID>> routeMap_;
    std::string layerBefore_;
    RouteCommonOptions routeOptions_;
  bool dirty_ = true;
};
};

}
