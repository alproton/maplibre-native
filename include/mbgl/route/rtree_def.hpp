#pragma once

#include "mbgl/util/geometry.hpp"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace mbgl {
namespace route {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::d2::point_xy<double> BoostPoint;      // For (longitude, latitude)
typedef bg::model::box<BoostPoint> BoostBox;             // MBR of a segment
typedef std::pair<BoostBox, size_t> RTreeValue;          // Store MBR and original segment index
typedef bgi::rtree<RTreeValue, bgi::rstar<16, 4>> RTree; // R*-tree variant, 16 max entries/node

} // namespace route
} // namespace mbgl
