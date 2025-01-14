//
// Created by spalaniappan on 1/6/25.
//

#include <mbgl/route/route_options.hpp>

#pragma once

namespace mbgl {
namespace gfx {
class Route {
public:
    Route(const RouteOptions& routeOptions);
    ~Route();

private:
    RouteOptions options_;
};
} // namespace gfx
} // namespace mbgl
