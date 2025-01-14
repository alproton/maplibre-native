
//
// Created by spalaniappan on 1/6/25.
//

#pragma once

#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>

namespace mbgl {
namespace gfx {
struct RouteOptions {
    Color innerColor;
    Color outerColor;
    LineString<double> lineString;
};
} // namespace gfx
} // namespace mbgl
