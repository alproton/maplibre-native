#pragma once

namespace mbgl {
namespace route {

enum class Precision {
    Coarse,
    Fine,
    Mercator
};

enum class RouteType {
    Casing,
    Inner
};

} // namespace route
} // namespace mbgl
