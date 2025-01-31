#pragma once

#include <memory>
#include <mbgl/route/id_types.hpp>
#include <mbgl/route/RouteSegment.hpp>
#include <mbgl/route/make_id.hpp>
#include <memory>
#include <unordered_map>
#include <map>

namespace mbgl {


namespace style {
class Style;
} // namespace style

namespace gfx {

/***
 * A route is a road between two locations. There can be multiple routes with multiple destinations. Each route can also have route segments. Routes segments can be for traffic data or any other overlay information.
 *
 */
class RouteManager final {
public:
  static RouteManager& getInstance() noexcept;
  void setStyle(style::Style&);
  RouteID routeCreate();
  RouteSegmentID routeSegmentCreate(const RouteID&, const gfx::RouteSegmentOptions&, const RouteSegmentHint& segmentHint);
  bool routeSegmentUpdate(const RouteID&, const RouteSegmentID&, gfx::RouteSegmentOptions&);
  bool routeSegmentDispose(const RouteID&, const RouteSegmentID&);
  bool routeDispose(const RouteID&);

  ~RouteManager();
private:

  RouteManager();
  IDpool routeIDpool_ = IDpool(100);
  IDpool routeSegmentIDpool_ = IDpool(100);
  style::Style* style_ = nullptr;
  void finalize();
  std::unordered_map<RouteID, std::map<RouteSegmentID, RouteSegment>, IDHasher<RouteID>> routeMap_;
  bool finalized_ = false;
};


};

}
