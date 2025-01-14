//
// Created by spalaniappan on 1/6/25.
//

#include <memory>

#pragma once

namespace mbgl {

namespace style {
class Style;
} // namespace style

class RouteManager final {
    public:
        RouteManager(style::Style&);
        void setStyle(style::Style&);
        ~RouteManager();
    private:
    std::reference_wrapper<style::Style> style;
};

}
