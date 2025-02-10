#pragma once

#include <memory>
#include <mbgl/route/id_types.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/route/id_pool.hpp>
#include <memory>
#include <unordered_map>
#include <map>

namespace mbgl {


namespace style {
class Style;
} // namespace style

namespace route {

/***
 * A route is a road between two locations. There can be multiple routes in the case of multiple stops.
 * Each route can have route segments. Routes segments can be used for traffic data.
 * A route must have a base route segment that contains all the vertices of the polyline that makes it.
 */
class RouteManager final {
public:
  static RouteManager& getInstance() noexcept;
  void setStyle(style::Style&);
  RouteID routeCreate();
  RouteSegmentID routeSegmentCreate(const RouteID&, const RouteSegmentOptions&);
  bool routeSegmentUpdate(const RouteID&, const RouteSegmentID&, RouteSegmentOptions&);
  bool routeSegmentDispose(const RouteID&, const RouteSegmentID&);
  bool routeDispose(const RouteID&);
  void finalize();

  ~RouteManager();
private:

  RouteManager();
  gfx::IDpool routeIDpool_ = gfx::IDpool(100);
  gfx::IDpool routeSegmentIDpool_ = gfx::IDpool(100);
  style::Style* style_ = nullptr;
  std::unordered_map<RouteID, std::map<RouteSegmentID, route::RouteSegment>, IDHasher<RouteID>> routeMap_;
  bool finalized_ = false;
};
};

}
