#include <mbgl/test/util.hpp>

#include <mbgl/route/route.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/util/geometry.hpp>

using namespace mbgl;
using namespace mbgl::route;

namespace {

LineString<double> makeSimpleGeometry(size_t numPoints) {
    LineString<double> geom;
    for (size_t i = 0; i < numPoints; ++i) {
        geom.push_back(Point<double>{static_cast<double>(i), 0.0});
    }
    return geom;
}

} // namespace

TEST(Route, SegmentCreateValidIndices) {
    auto geom = makeSimpleGeometry(10);
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 0;
    segOpts.firstIndexFraction = 0.0f;
    segOpts.lastIndex = 5;
    segOpts.lastIndexFraction = 0.5f;

    EXPECT_TRUE(route.routeSegmentCreate(segOpts));
    EXPECT_TRUE(route.hasRouteSegments());
}

TEST(Route, SegmentCreateLastValidIndex) {
    auto geom = makeSimpleGeometry(10);
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 0;
    segOpts.firstIndexFraction = 0.0f;
    segOpts.lastIndex = 8; // geometry has 10 points, so 9 intervals (indices 0..8)
    segOpts.lastIndexFraction = 1.0f;

    EXPECT_TRUE(route.routeSegmentCreate(segOpts));
}

TEST(Route, SegmentCreateOutOfBoundsFirstIndex) {
    auto geom = makeSimpleGeometry(10);
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 9; // out of bounds: only 9 intervals (0..8)
    segOpts.firstIndexFraction = 0.0f;
    segOpts.lastIndex = 5;
    segOpts.lastIndexFraction = 0.5f;

    EXPECT_FALSE(route.routeSegmentCreate(segOpts));
}

TEST(Route, SegmentCreateOutOfBoundsLastIndex) {
    auto geom = makeSimpleGeometry(10);
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 0;
    segOpts.firstIndexFraction = 0.0f;
    segOpts.lastIndex = 100; // way out of bounds
    segOpts.lastIndexFraction = 0.5f;

    EXPECT_FALSE(route.routeSegmentCreate(segOpts));
}

TEST(Route, SegmentCreateBothIndicesOutOfBounds) {
    auto geom = makeSimpleGeometry(10);
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 148; // reproduces the crash scenario from tombstone analysis
    segOpts.firstIndexFraction = 0.5f;
    segOpts.lastIndex = 200;
    segOpts.lastIndexFraction = 0.5f;

    EXPECT_FALSE(route.routeSegmentCreate(segOpts));
}

TEST(Route, SegmentCreateReversedIndices) {
    auto geom = makeSimpleGeometry(10);
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 5;
    segOpts.firstIndexFraction = 0.0f;
    segOpts.lastIndex = 2;
    segOpts.lastIndexFraction = 0.5f;

    EXPECT_FALSE(route.routeSegmentCreate(segOpts));
}

TEST(Route, SegmentCreateSameIndexReversedFractions) {
    auto geom = makeSimpleGeometry(10);
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 3;
    segOpts.firstIndexFraction = 0.8f;
    segOpts.lastIndex = 3;
    segOpts.lastIndexFraction = 0.2f;

    EXPECT_FALSE(route.routeSegmentCreate(segOpts));
}

TEST(Route, SegmentCreateSameIndexValidFractions) {
    auto geom = makeSimpleGeometry(10);
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 3;
    segOpts.firstIndexFraction = 0.2f;
    segOpts.lastIndex = 3;
    segOpts.lastIndexFraction = 0.8f;

    EXPECT_TRUE(route.routeSegmentCreate(segOpts));
}

TEST(Route, SegmentCreateOnMinimalGeometry) {
    auto geom = makeSimpleGeometry(2); // minimal valid geometry: 2 points, 1 interval
    RouteOptions opts;
    Route route(geom, opts);

    RouteSegmentOptions validSeg;
    validSeg.firstIndex = 0;
    validSeg.firstIndexFraction = 0.0f;
    validSeg.lastIndex = 0;
    validSeg.lastIndexFraction = 1.0f;
    EXPECT_TRUE(route.routeSegmentCreate(validSeg));

    route.routeSegmentsClear();

    RouteSegmentOptions oobSeg;
    oobSeg.firstIndex = 1; // out of bounds: only 1 interval (index 0)
    oobSeg.firstIndexFraction = 0.0f;
    oobSeg.lastIndex = 0;
    oobSeg.lastIndexFraction = 0.5f;
    EXPECT_FALSE(route.routeSegmentCreate(oobSeg));
}
