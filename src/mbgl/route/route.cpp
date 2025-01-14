//
// Created by spalaniappan on 1/6/25.
//

#include <mbgl/route/route.hpp>

namespace mbgl {
namespace gfx {
Route::Route(const RouteOptions& routeOptions)
    : options_(routeOptions) {}
Route::~Route() {}
} // namespace gfx
} // namespace mbgl
