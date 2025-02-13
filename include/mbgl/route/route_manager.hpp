#pragma once

#include <mbgl/route/id_types.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/route/id_pool.hpp>
#include <unordered_map>
namespace mbgl {


namespace style {
class Style;
} // namespace style

namespace route {

class Route;
// class LineLayer;


struct RouteCommonOptions {
    Color outerColor = Color(1, 1, 1, 1);
    Color innerColor = Color(0, 0, 1, 1);
    float outerWidth = 10;
    float innerWidth = 6;
};


/***
 * A route is a road between two locations. There can be multiple routes in the case of multiple stops.
 * Each route can have route segments. Routes segments can be used for traffic data.
 * A route must have a base route segment that contains all the vertices of the polyline that makes it.
 */
class RouteManager final {
public:
    static const std::string BASE_ROUTE_SEGMENT_STR;

  static RouteManager& getInstance() noexcept;
  void setStyle(style::Style&);
  bool hasStyle() const;
  void setLayerBefore(const std::string layerBefore);
  void setRouteCommonOptions(const RouteCommonOptions& ropts);
  RouteID routeCreate(const LineString<double>& geometry);
  void routeSegmentCreate(const RouteID&, const RouteSegmentOptions&);
  bool routeDispose(const RouteID&);
    bool hasRoutes() const;
  void finalize();

  ~RouteManager();
private:

  RouteManager();
    static const std::string BASE_ROUTE_LAYER;
    static const std::string ACTIVE_ROUTE_LAYER;
    static const std::string GEOJSON_ROUTE_SOURCE_ID;

  gfx::IDpool routeIDpool_ = gfx::IDpool(100);
  style::Style* style_ = nullptr;
  std::unordered_map<RouteID, Route, IDHasher<RouteID>> routeMap_;
    std::string layerBefore_;
    RouteCommonOptions routeOptions_;
  bool dirty_ = true;
};
};

}
