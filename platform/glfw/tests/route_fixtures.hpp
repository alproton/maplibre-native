#pragma once
#include <mbgl/route/id_types.hpp>
#include <string>
#include <mbgl/util/color.hpp>
#include <mbgl/util/geometry.hpp>
#include <vector>

namespace route_fixtures {

struct TrafficBlock {
    mbgl::LineString<double> block;
    uint32_t firstIndex = INVALID_UINT;
    float firstIndexFraction = 0.0f;
    uint32_t lastIndex = INVALID_UINT;
    float lastIndexFraction = 0.0f;
    uint32_t priority = 0;
    mbgl::Color color;
};

struct RouteData {
    double resolution = 10;
    double xlate = 0;
    double radius = 5;
    int numTrafficZones = 5;
    bool trafficZonesGridAligned = true;
    mbgl::LineString<double> points;

    mbgl::Point<double> getPoint(double percent) const;
    std::pair<uint32_t, double> getIntervalFraction(double percent) const;
};

inline mbgl::Color convert(std::string hexcolor) {
    std::stringstream ss;
    ss << std::hex << hexcolor;
    int color;
    ss >> color;
    float r = (color >> 16) & 0xFF;
    float g = (color >> 8) & 0xFF;
    float b = (color) & 0xFF;
    float a = 1.0f;
    return {mbgl::Color(r / 255.0f, g / 255.0f, b / 255.0f, a)};
}

enum RouteColorType {
    RouteMapAlternative,
    RouteMapAlternativeCasing,
    RouteMapAlternativeLowTrafficColor,
    RouteMapAlternativeModerateTrafficColor,
    RouteMapAlternativeHeavyTrafficColor,
    RouteMapAlternativeSevereTrafficColor,
    RouteMapColor,
    RouteMapCasingColor,
    RouteMapLowTrafficColor,
    RouteMapModerateTrafficColor,
    RouteMapHeavyTrafficColor,
    RouteMapSevereTrafficColor,
    InactiveLegRouteColor,
    InactiveRouteLowTrafficColor,
    InactiveRouteModerateTrafficColor,
    InactiveRouteHeavyTrafficColor,
    InactiveRouteSevereTrafficColor
};

const std::unordered_map<RouteColorType, mbgl::Color> routeColorTable = {
    {RouteMapAlternative, convert("7A7A7A")},
    {RouteMapAlternativeCasing, convert("FFFFFF")},
    {RouteMapAlternativeLowTrafficColor, convert("FFCC5B")},
    {RouteMapAlternativeModerateTrafficColor, convert("F0691D")},
    {RouteMapAlternativeHeavyTrafficColor, convert("DB0000")},
    {RouteMapAlternativeSevereTrafficColor, convert("9B0000")},
    {RouteMapColor, convert("2F70A9")},
    {RouteMapCasingColor, convert("FFFFFF")},
    {RouteMapLowTrafficColor, convert("FFBC2D")},
    {RouteMapModerateTrafficColor, convert("ED6D4A")},
    {RouteMapHeavyTrafficColor, convert("DB0000")},
    {RouteMapSevereTrafficColor, convert("9B0000")},
    {InactiveLegRouteColor, convert("76A7D1")},
    {InactiveRouteLowTrafficColor, convert("FFE5AD")},
    {InactiveRouteModerateTrafficColor, convert("F39F7E")},
    {InactiveRouteHeavyTrafficColor, convert("EE7676")},
    {InactiveRouteSevereTrafficColor, convert("E64747")}};

inline std::vector<mbgl::Color> getActiveColors() {
    return {routeColorTable.at(RouteColorType::RouteMapLowTrafficColor),
            routeColorTable.at(RouteColorType::RouteMapModerateTrafficColor),
            routeColorTable.at(RouteColorType::RouteMapHeavyTrafficColor),
            routeColorTable.at(RouteColorType::RouteMapSevereTrafficColor)};
};

inline std::vector<mbgl::Color> getAlternativeColors() {
    return {routeColorTable.at(RouteColorType::InactiveRouteLowTrafficColor),
            routeColorTable.at(RouteColorType::InactiveRouteModerateTrafficColor),
            routeColorTable.at(RouteColorType::InactiveRouteHeavyTrafficColor),
            routeColorTable.at(RouteColorType::InactiveRouteHeavyTrafficColor)};
};

} // namespace route_fixtures
