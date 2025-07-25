#include "route_traffic_priority_test.hpp"
#include "../glfw_view.hpp"

RouteTrafficPriorityTest::RouteTrafficPriorityTest(const std::string& testDir)
    : RouteTest("route_traffic_priority_test", testDir) {}

bool RouteTrafficPriorityTest::produceTestCommands([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
    assert(map != nullptr && "invalid map!");
    bool success = false;
    if (map != nullptr) {
        using namespace mbgl::route;
        using namespace route_fixtures;
        testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
            map->setDebug(mbgl::MapDebugOptions::NoDebug);
            map->jumpTo(mbgl::CameraOptions().withCenter(mbgl::LatLng{}).withZoom(2).withBearing(0.0).withPitch(0.0));

            mbgl::Color color0 = routeColorTable.at(RouteColorType::RouteMapColor);
            mbgl::Color color1 = routeColorTable.at(RouteColorType::RouteMapAlternative);
            std::vector<mbgl::Color> colors = {color0, color1};

            auto getRouteGeom = [](float radius, int resolution, float xlate) -> mbgl::LineString<double> {
                mbgl::LineString<double> linestring;
                for (int i = 0; i < resolution; i++) {
                    float anglerad = (float(i) / float(resolution - 1)) * 2 * 3.14f;
                    mbgl::Point<double> pt{xlate + radius * sin(anglerad), radius * cos(anglerad)};
                    linestring.push_back(pt);
                }

                return linestring;
            };

            RouteData rd;
            rd.radius = 50.0f;
            rd.resolution = 10;
            rd.xlate = 0.0;
            rd.points = getRouteGeom(rd.radius, rd.resolution, rd.xlate);
            RouteOptions routeOpts;
            routeOpts.innerColor = color0;
            routeOpts.outerColor = mbgl::Color(0.2, 0.2, 0.2, 1);
            routeOpts.useDynamicWidths = false;
            routeOpts.outerClipColor = mbgl::Color(0.5, 0.5, 0.5, 1.0);
            routeOpts.innerClipColor = mbgl::Color(0.5, 0.5, 0.5, 1.0);
            routeOpts.useMercatorProjection = false;

            const RouteID& routeID = rmptr_->routeCreate(rd.points, routeOpts);
            assert(routeID.isValid() && "invalid route ID created");
            rmptr_->finalize();
            routeMap_[routeID] = rd;
        });

        testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
            setTrafficCase(RouteSegmentTestCases::Blk1HighPriorityIntersecting);
        });

        testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
            setTrafficCase(RouteSegmentTestCases::Blk1LowPriorityIntersecting);
        });

        testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
            setTrafficCase(RouteSegmentTestCases::Blk12NonIntersecting);
        });

        testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
            setTrafficCase(RouteSegmentTestCases::Blk12SameColorIntersecting);
        });

        success = true;
    }

    return success;
}

void RouteTrafficPriorityTest::setTrafficCase(const RouteSegmentTestCases& testcase) const {
    using namespace route_fixtures;
    TrafficBlock block1;
    TrafficBlock block2;
    const RouteID& routeID = routeMap_.begin()->first;
    const RouteData& route = routeMap_.at(routeID);
    bool useFractionalRouteSegments = true;
    const auto& routeTrafficUpdate = [&](TrafficBlock& block,
                                         std::pair<uint32_t, double> firstIndexFraction,
                                         std::pair<uint32_t, double> lastIndexFraction) {
        block.firstIndex = firstIndexFraction.first;
        block.firstIndexFraction = firstIndexFraction.second;
        block.lastIndex = lastIndexFraction.first;
        block.lastIndexFraction = lastIndexFraction.second;
    };

    switch (testcase) {
        case RouteSegmentTestCases::Blk1LowPriorityIntersecting: {
            if (!useFractionalRouteSegments) {
                block1.block = {route.getPoint(0.0), route.getPoint(0.25), route.getPoint(0.5)};
            } else {
                std::pair<uint32_t, double> firstIndexFraction1 = route.getIntervalFraction(0.0);
                std::pair<uint32_t, double> lastIndexFraction1 = route.getIntervalFraction(0.5);
                routeTrafficUpdate(block1, firstIndexFraction1, lastIndexFraction1);
            }

            block1.priority = 0;
            block1.color = routeColorTable.at(RouteMapLowTrafficColor);

            if (!useFractionalRouteSegments) {
                block2.block = {route.getPoint(0.2), route.getPoint(0.7), route.getPoint(0.8)};
            } else {
                std::pair<uint32_t, double> firstIndexFraction2 = route.getIntervalFraction(0.2);
                std::pair<uint32_t, double> lastIndexFraction2 = route.getIntervalFraction(0.8);
                routeTrafficUpdate(block2, firstIndexFraction2, lastIndexFraction2);
            }

            block2.priority = 1;
            block2.color = routeColorTable.at(RouteMapModerateTrafficColor);
        } break;

        case RouteSegmentTestCases::Blk1HighPriorityIntersecting: {
            if (!useFractionalRouteSegments) {
                block1.block = {route.getPoint(0.0), route.getPoint(0.25), route.getPoint(0.5)};
            } else {
                std::pair<uint32_t, double> firstIndexFraction1 = route.getIntervalFraction(0.0);
                std::pair<uint32_t, double> lastIndexFraction1 = route.getIntervalFraction(0.5);
                routeTrafficUpdate(block1, firstIndexFraction1, lastIndexFraction1);
            }
            block1.priority = 1;
            block1.color = routeColorTable.at(RouteMapLowTrafficColor);

            if (!useFractionalRouteSegments) {
                block2.block = {route.getPoint(0.2), route.getPoint(0.7), route.getPoint(0.8)};
            } else {
                std::pair<uint32_t, double> firstIndexFraction2 = route.getIntervalFraction(0.2);
                std::pair<uint32_t, double> lastIndexFraction2 = route.getIntervalFraction(0.8);
                routeTrafficUpdate(block2, firstIndexFraction2, lastIndexFraction2);
            }
            block2.priority = 0;
            block2.color = routeColorTable.at(RouteMapModerateTrafficColor);
        } break;

        case RouteSegmentTestCases::Blk12SameColorIntersecting: {
            if (!useFractionalRouteSegments) {
                block1.block = {route.getPoint(0.0), route.getPoint(0.25), route.getPoint(0.5)};
            } else {
                std::pair<uint32_t, double> firstIndexFraction1 = route.getIntervalFraction(0.0);
                std::pair<uint32_t, double> lastIndexFraction1 = route.getIntervalFraction(0.5);
                routeTrafficUpdate(block1, firstIndexFraction1, lastIndexFraction1);
            }
            block1.priority = 0;
            block1.color = routeColorTable.at(RouteMapLowTrafficColor);

            if (!useFractionalRouteSegments) {
                block2.block = {route.getPoint(0.2), route.getPoint(0.7), route.getPoint(0.8)};
            } else {
                std::pair<uint32_t, double> firstIndexFraction2 = route.getIntervalFraction(0.2);
                std::pair<uint32_t, double> lastIndexFraction2 = route.getIntervalFraction(0.8);
                routeTrafficUpdate(block2, firstIndexFraction2, lastIndexFraction2);
            }
            block2.priority = 1;
            block2.color = routeColorTable.at(RouteMapLowTrafficColor);

        } break;

        case RouteSegmentTestCases::Blk12NonIntersecting: {
            if (!useFractionalRouteSegments) {
                block1.block = {route.getPoint(0.0), route.getPoint(0.25), route.getPoint(0.5)};
            } else {
                std::pair<uint32_t, double> firstIndexFraction1 = route.getIntervalFraction(0.0);
                std::pair<uint32_t, double> lastIndexFraction1 = route.getIntervalFraction(0.5);
                routeTrafficUpdate(block1, firstIndexFraction1, lastIndexFraction1);
            }
            block1.priority = 0;
            block1.color = routeColorTable.at(RouteMapLowTrafficColor);

            if (!useFractionalRouteSegments) {
                block2.block = {route.getPoint(0.6), route.getPoint(0.7), route.getPoint(0.8)};
            } else {
                std::pair<uint32_t, double> firstIndexFraction2 = route.getIntervalFraction(0.6);
                std::pair<uint32_t, double> lastIndexFraction2 = route.getIntervalFraction(0.8);
                routeTrafficUpdate(block2, firstIndexFraction2, lastIndexFraction2);
            }
            block2.priority = 0;
            block2.color = routeColorTable.at(RouteMapModerateTrafficColor);

        } break;

        default:
            break;
    }

    std::vector<TrafficBlock> fixture{block1, block2};
    rmptr_->routeClearSegments(routeID);
    rmptr_->setUseRouteSegmentIndexFractions(useFractionalRouteSegments);
    for (size_t i = 0; i < fixture.size(); i++) {
        mbgl::route::RouteSegmentOptions rsegopts;
        rsegopts.color = fixture[i].color;
        rsegopts.firstIndex = fixture[i].firstIndex;
        rsegopts.firstIndexFraction = fixture[i].firstIndexFraction;
        rsegopts.lastIndex = fixture[i].lastIndex;
        rsegopts.lastIndexFraction = fixture[i].lastIndexFraction;
        rsegopts.geometry = fixture[i].block;

        rsegopts.priority = fixture[i].priority;
        rsegopts.outerColor = mbgl::Color(float(i) / float(fixture.size() - 1), 0.0, 0.0, 1.0);
        const bool success = rmptr_->routeSegmentCreate(routeID, rsegopts);
        assert(success && "failed to create route segment");
    }
    fixture.clear();
    rmptr_->finalize();
}

RouteTrafficPriorityTest::~RouteTrafficPriorityTest() {}
