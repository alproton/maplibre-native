
#pragma once


#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <string>
#include <mbgl/route/id_types.hpp>
#include <mbgl/route/id_pool.hpp>

namespace mbgl {
namespace route {

class RouteSegment;
struct RouteSegmentOptions;

class Route {
    public:
    Route();
    void routeSegmentCreate(const RouteSegmentOptions&);
    mbgl::LineString<double> getGeometry(const std::string& name) const;
    bool clear();
    bool getDirty() const;
    Route& operator=(Route& other) noexcept;

private:
    bool dirty_ = true;
    //keep the geometry/route segment ordered by the type of traffic or base geometry names for easy access.
    std::unordered_map<std::string, RouteSegment> segments_;
};

} // namespace gfx
} // namespace mbgl
