#include <mbgl/test/util.hpp>

#include <mbgl/route/route.hpp>
#include <mbgl/route/route_manager.hpp>
#include <mbgl/route/route_segment.hpp>
#include <mbgl/route/route_enums.hpp>
#include <mbgl/shaders/line_layer_ubo.hpp>
#include <mbgl/style/layers/line_layer_impl.hpp>
#include <mbgl/util/color.hpp>

#include <cstddef>
#include <vector>

using namespace mbgl;
using namespace mbgl::route;
using namespace mbgl::shaders;

namespace {

LineString<double> makeCircleGeometry(float radius, int resolution) {
    LineString<double> line;
    for (int i = 0; i < resolution; i++) {
        float angle = (static_cast<float>(i) / static_cast<float>(resolution - 1)) * 2.0f * 3.14159f;
        line.push_back({radius * std::sin(angle), radius * std::cos(angle)});
    }
    return line;
}

Route makeTestRoute(int numPoints = 10, float radius = 50.0f) {
    RouteOptions opts;
    opts.useMercatorProjection = false;
    return Route(makeCircleGeometry(radius, numPoints), opts);
}

} // namespace

// 1. compactSegments returns empty when no segments are added
TEST(RouteTraffic, CompactSegmentsEmpty) {
    Route route = makeTestRoute();
    auto segments = route.compactSegments(RouteType::Inner);
    EXPECT_TRUE(segments.empty());
}

// 2. compactSegments with a single segment
TEST(RouteTraffic, CompactSegmentsSingle) {
    Route route = makeTestRoute();

    RouteSegmentOptions segOpts;
    segOpts.firstIndex = 0;
    segOpts.firstIndexFraction = 0.0f;
    segOpts.lastIndex = 0;
    segOpts.lastIndexFraction = 1.0f;
    segOpts.color = Color(1.0f, 0.0f, 0.0f, 1.0f);
    segOpts.priority = 0;

    EXPECT_TRUE(route.routeSegmentCreate(segOpts));

    auto segments = route.compactSegments(RouteType::Inner);
    ASSERT_EQ(segments.size(), 1u);
    EXPECT_GT(segments[0].range.second, segments[0].range.first);
    EXPECT_GE(segments[0].range.first, 0.0);
    EXPECT_LE(segments[0].range.second, 1.0);
}

// 3. compactSegments with multiple non-overlapping segments
TEST(RouteTraffic, CompactSegmentsMultipleNonOverlapping) {
    Route route = makeTestRoute();

    RouteSegmentOptions seg0;
    seg0.firstIndex = 0;
    seg0.firstIndexFraction = 0.0f;
    seg0.lastIndex = 0;
    seg0.lastIndexFraction = 1.0f;
    seg0.color = Color(1.0f, 0.0f, 0.0f, 1.0f);
    seg0.priority = 0;

    RouteSegmentOptions seg1;
    seg1.firstIndex = 3;
    seg1.firstIndexFraction = 0.0f;
    seg1.lastIndex = 3;
    seg1.lastIndexFraction = 1.0f;
    seg1.color = Color(0.0f, 1.0f, 0.0f, 1.0f);
    seg1.priority = 0;

    RouteSegmentOptions seg2;
    seg2.firstIndex = 6;
    seg2.firstIndexFraction = 0.0f;
    seg2.lastIndex = 6;
    seg2.lastIndexFraction = 1.0f;
    seg2.color = Color(0.0f, 0.0f, 1.0f, 1.0f);
    seg2.priority = 0;

    EXPECT_TRUE(route.routeSegmentCreate(seg0));
    EXPECT_TRUE(route.routeSegmentCreate(seg1));
    EXPECT_TRUE(route.routeSegmentCreate(seg2));

    auto segments = route.compactSegments(RouteType::Inner);
    ASSERT_EQ(segments.size(), 3u);

    for (size_t i = 0; i < segments.size(); i++) {
        EXPECT_GT(segments[i].range.second, segments[i].range.first);
        EXPECT_GE(segments[i].range.first, 0.0);
        EXPECT_LE(segments[i].range.second, 1.0);
    }
    EXPECT_LT(segments[0].range.second, segments[1].range.first);
    EXPECT_LT(segments[1].range.second, segments[2].range.first);
}

// 4. compactSegments with overlapping segments and priority
TEST(RouteTraffic, CompactSegmentsOverlappingPriority) {
    Route route = makeTestRoute();

    RouteSegmentOptions segLow;
    segLow.firstIndex = 1;
    segLow.firstIndexFraction = 0.0f;
    segLow.lastIndex = 3;
    segLow.lastIndexFraction = 1.0f;
    segLow.color = Color(1.0f, 0.0f, 0.0f, 1.0f);
    segLow.priority = 0;

    RouteSegmentOptions segHigh;
    segHigh.firstIndex = 2;
    segHigh.firstIndexFraction = 0.0f;
    segHigh.lastIndex = 4;
    segHigh.lastIndexFraction = 1.0f;
    segHigh.color = Color(0.0f, 1.0f, 0.0f, 1.0f);
    segHigh.priority = 1;

    EXPECT_TRUE(route.routeSegmentCreate(segLow));
    EXPECT_TRUE(route.routeSegmentCreate(segHigh));

    auto segments = route.compactSegments(RouteType::Inner);
    ASSERT_GE(segments.size(), 2u);

    for (const auto& seg : segments) {
        EXPECT_GT(seg.range.second, seg.range.first);
        EXPECT_GE(seg.range.first, 0.0);
        EXPECT_LE(seg.range.second, 1.0);
    }
}

// 5. RouteManager toggle API
TEST(RouteTraffic, HighPrecisionToggle) {
    RouteManager rm;
    EXPECT_FALSE(rm.getUseHighPrecisionTraffic());

    rm.setUseHighPrecisionTraffic(true);
    EXPECT_TRUE(rm.getUseHighPrecisionTraffic());

    rm.setUseHighPrecisionTraffic(false);
    EXPECT_FALSE(rm.getUseHighPrecisionTraffic());
}

// 6. UBO struct layout matches expected sizes and offsets
TEST(RouteTraffic, UBOStructLayout) {
    EXPECT_EQ(sizeof(RouteTrafficSegmentData), 32u);
    EXPECT_EQ(sizeof(LineTrafficSegmentsUBO), 65u * 16u);
    EXPECT_EQ(offsetof(LineTrafficSegmentsUBO, useHighPrecision), 1024u);
    EXPECT_EQ(offsetof(LineTrafficSegmentsUBO, segmentCount), 1028u);
}

// 7. UBO population logic
TEST(RouteTraffic, UBOPopulation) {
    std::vector<style::TrafficSegmentData> input = {
        {0.1, 0.3, Color(1.0f, 0.0f, 0.0f, 1.0f)},
        {0.5, 0.7, Color(0.0f, 1.0f, 0.0f, 1.0f)},
        {0.8, 0.95, Color(0.0f, 0.0f, 1.0f, 1.0f)},
    };

    LineTrafficSegmentsUBO ubo{};
    ubo.useHighPrecision = 1;
    ubo.segmentCount = std::min(static_cast<int>(input.size()), MAX_ROUTE_TRAFFIC_SEGMENTS);

    for (int i = 0; i < ubo.segmentCount; i++) {
        ubo.segments[i].start = static_cast<float>(input[i].start);
        ubo.segments[i].end = static_cast<float>(input[i].end);
        ubo.segments[i].color = {input[i].color.r, input[i].color.g, input[i].color.b, input[i].color.a};
    }

    EXPECT_EQ(ubo.useHighPrecision, 1);
    EXPECT_EQ(ubo.segmentCount, 3);

    EXPECT_FLOAT_EQ(ubo.segments[0].start, 0.1f);
    EXPECT_FLOAT_EQ(ubo.segments[0].end, 0.3f);
    EXPECT_FLOAT_EQ(ubo.segments[0].color[0], 1.0f);
    EXPECT_FLOAT_EQ(ubo.segments[0].color[1], 0.0f);

    EXPECT_FLOAT_EQ(ubo.segments[1].start, 0.5f);
    EXPECT_FLOAT_EQ(ubo.segments[1].end, 0.7f);
    EXPECT_FLOAT_EQ(ubo.segments[1].color[1], 1.0f);

    EXPECT_FLOAT_EQ(ubo.segments[2].start, 0.8f);
    EXPECT_FLOAT_EQ(ubo.segments[2].end, 0.95f);
    EXPECT_FLOAT_EQ(ubo.segments[2].color[2], 1.0f);
}

// 8. MAX_ROUTE_TRAFFIC_SEGMENTS clamping
TEST(RouteTraffic, UBOSegmentClamping) {
    std::vector<style::TrafficSegmentData> input;
    for (int i = 0; i < MAX_ROUTE_TRAFFIC_SEGMENTS + 10; i++) {
        double start = static_cast<double>(i) / (MAX_ROUTE_TRAFFIC_SEGMENTS + 10);
        double end = start + 0.005;
        input.push_back({start, end, Color(1.0f, 0.0f, 0.0f, 1.0f)});
    }

    LineTrafficSegmentsUBO ubo{};
    ubo.segmentCount = std::min(static_cast<int>(input.size()), MAX_ROUTE_TRAFFIC_SEGMENTS);

    for (int i = 0; i < ubo.segmentCount; i++) {
        ubo.segments[i].start = static_cast<float>(input[i].start);
        ubo.segments[i].end = static_cast<float>(input[i].end);
        ubo.segments[i].color = {input[i].color.r, input[i].color.g, input[i].color.b, input[i].color.a};
    }

    EXPECT_EQ(ubo.segmentCount, MAX_ROUTE_TRAFFIC_SEGMENTS);
    EXPECT_EQ(static_cast<int>(input.size()), MAX_ROUTE_TRAFFIC_SEGMENTS + 10);
}
