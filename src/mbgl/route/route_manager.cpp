//
// Created by spalaniappan on 1/6/25.
//
#include <mbgl/route/route_manager.hpp>

namespace mbgl {

RouteManager::RouteManager(style::Style& style_)
    : style(style_) {}

void RouteManager::setStyle(style::Style& style_) {
    style = style_;
}

RouteManager::~RouteManager() {}

} // namespace mbgl
