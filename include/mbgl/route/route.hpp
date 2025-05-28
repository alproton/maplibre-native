
#pragma once

#include "mbgl/util/geo.hpp"

#include <mbgl/route/route_segment.hpp>

#include <mbgl/util/geometry.hpp>
#include <mbgl/util/color.hpp>
#include <mbgl/route/route_enums.hpp>
#include <vector>
#include <map>
#include <algorithm>

namespace mbgl {
namespace route {

struct RouteSegmentOptions;
class RouteSegment;

struct RouteOptions {
    Color outerColor = Color(1, 1, 1, 1);
    Color innerColor = Color(0, 0, 1, 1);
    Color outerClipColor = Color(0, 0, 0, 0);
    Color innerClipColor = Color(0, 0, 0, 0);
    float outerWidth = 16;
    float innerWidth = 14;
    std::map<double, double> outerWidthZoomStops;
    std::map<double, double> innerWidthZoomStops;
    bool useDynamicWidths = false;
    std::string layerBefore;
};

/***
 * A route is a polyline that represents a road between two locations. There can be multiple routes in the case of
 * multiple stops. Each route can have route segments. Routes segments represents a traffic zone. A route must have a
 * base route segment (aka casing) that contains all the vertices of the polyline that makes it.
 */
class Route {
public:
    Route() = default;
    Route(const LineString<double>& geometry, const RouteOptions& ropts);
    bool routeSegmentCreate(const RouteSegmentOptions&);
    std::map<double, mbgl::Color> getRouteSegmentColorStops(const RouteType& routeType, const mbgl::Color& routeColor);
    std::map<double, mbgl::Color> getRouteColorStops(const mbgl::Color& routeColor) const;
    std::vector<double> getRouteSegmentDistances() const;
    void routeSetProgress(const double t, bool capture = false);
    double routeGetProgress() const;
    double getTotalDistance() const;
    double getProgressPercent(const Point<double>& queryPoint, const Precision& precision, bool capture = false);
    Point<double> getPoint(double percent, const Precision& precision, double* bearing = nullptr) const;
    double getProgressPassthrough(uint32_t intervalIndex, double intervalFraction) const;

    mbgl::LineString<double> getGeometry() const;
    bool hasRouteSegments() const;
    const RouteOptions& getRouteOptions() const;
    bool routeSegmentsClear();
    uint32_t getNumRouteSegments() const;
    const std::vector<Point<double>>& getCapturedNavStops() const;
    const std::vector<double>& getCapturedNavPercent() const;
    static std::optional<double> getVectorOrientation(double x, double y);

    std::string segmentsToString(uint32_t tabcount) const;

private:
    struct SegmentRange {
        std::pair<double, double> range;
        Color color;
    };

    std::vector<SegmentRange> compactSegments(const RouteType& routeType) const;
    Point<double> getPointCoarse(double percent, double* bearing = nullptr) const;
    Point<double> getPointFine(double percent, double* bearing = nullptr) const;
    double getProgressProjectionLERP(const Point<double>& queryPoint, bool capture = false);
    double getProgressProjectionSLERP(const Point<double>& queryPoint, bool capture = false);

    RouteOptions routeOptions_;
    double progress_ = 0.0;
    std::vector<double> intervalLengths_;
    std::vector<double> cumulativeIntervalDistances_;
    std::vector<RouteSegment> segments_;
    mbgl::LineString<double> geometry_;
    std::map<double, mbgl::Color> segGradient_;
    double totalLength_ = 0.0;
    std::vector<double> capturedNavPercent_;
    std::vector<Point<double>> capturedNavStops_;
    bool logPrecision = true;
};

} // namespace route
} // namespace mbgl
