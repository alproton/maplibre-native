#pragma once
#include <mbgl/util/math.hpp>
#include <mbgl/util/geometry.hpp>

namespace mbgl {
namespace route {

class RouteUtils final {
public:

    /**
    * Function to check if point A is between points B and C
    */
    static bool isPointBetween(const mbgl::Point<double>& A, const mbgl::Point<double>& B, const mbgl::Point<double>& C);
};

}
}
