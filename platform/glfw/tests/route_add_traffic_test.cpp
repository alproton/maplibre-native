#include "route_add_traffic_test.hpp"
#include <mbgl/style/style.hpp>
#include "route_fixtures.hpp"
#include "../glfw_view.hpp"

RouteAddTrafficTest::RouteAddTrafficTest(const std::string &testDir)
    : RouteTest("route_add_traffic_test", testDir) {}

bool RouteAddTrafficTest::produceTestCommands([[maybe_unused]] mbgl::Map *map, [[maybe_unused]] GLFWView *view) {
    assert(map != nullptr && "invalid map!");
    bool success = false;
    if (map != nullptr) {
        using namespace mbgl::route;
        using namespace route_fixtures;

        testCommands_.push([&]([[maybe_unused]] mbgl::Map *map, [[maybe_unused]] GLFWView *view) {
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

            RouteID routeID = rmptr_->routeCreate(rd.points, routeOpts);
            assert(routeID.isValid() && "invalid route ID created");
            rmptr_->finalize();

            routeMap_[routeID] = rd;
        });

        testCommands_.push([&]([[maybe_unused]] mbgl::Map *map, [[maybe_unused]] GLFWView *view) {
            std::vector<TrafficBlock> trafficBlks;
            for (const auto &iter : routeMap_) {
                const auto &routeID = iter.first;
                const auto &route = iter.second;
                std::vector<mbgl::Color> colors = routeID == routeMap_.begin()->first
                                                      ? route_fixtures::getActiveColors()
                                                      : route_fixtures::getAlternativeColors();

                // test case 1 - route segment extends wholly within the interval in first and last interval
                // test case 2 - route segment is completely within the interval
                // test case 3 - route segment is spans across into adjacent interval

                // test case 1
                TrafficBlock trafblk0;
                trafblk0.firstIndex = 0;
                trafblk0.firstIndexFraction = 0.0f;
                trafblk0.lastIndex = 0;
                trafblk0.lastIndexFraction = 1.0f;
                trafblk0.color = mbgl::Color(1.0f, 0.0f, 0.0f, 1.0f);

                TrafficBlock trafblk1;
                trafblk1.firstIndex = route.points.size() - 2; // second last point
                trafblk1.firstIndexFraction = 0.0f;
                trafblk1.lastIndex = route.points.size() - 2;
                trafblk1.lastIndexFraction = 1.0f;
                trafblk0.color = mbgl::Color(1.0f, 1.0f, 1.0f, 1.0f);

                // test case 2
                TrafficBlock trafblk2;
                trafblk2.firstIndex = 1;
                trafblk2.firstIndexFraction = 0.2f;
                trafblk2.lastIndex = 1;
                trafblk2.lastIndexFraction = 0.8f;
                trafblk2.color = mbgl::Color(0.0f, 1.0f, 0.0f, 1.0f);

                // test case 3
                TrafficBlock trafblk3;
                trafblk3.firstIndex = 2;
                trafblk3.firstIndexFraction = 0.5f;
                trafblk3.lastIndex = 3;
                trafblk3.lastIndexFraction = 0.5f;
                trafblk3.color = mbgl::Color(0.0f, 1.0f, 1.0f, 1.0f);

                trafficBlks.push_back(trafblk0);
                trafficBlks.push_back(trafblk1);
                trafficBlks.push_back(trafblk2);
                trafficBlks.push_back(trafblk3);

                // clear the route segments and create new ones from the traffic blocks
                rmptr_->routeClearSegments(routeID);
                for (size_t i = 0; i < trafficBlks.size(); i++) {
                    mbgl::route::RouteSegmentOptions rsegopts;
                    uint32_t coloridx = i % (trafficBlks.size() - 1);
                    rsegopts.color = colors[coloridx];
                    rsegopts.firstIndex = trafficBlks[i].firstIndex;
                    rsegopts.firstIndexFraction = trafficBlks[i].firstIndexFraction;
                    rsegopts.lastIndex = trafficBlks[i].lastIndex;
                    rsegopts.lastIndexFraction = trafficBlks[i].lastIndexFraction;

                    rsegopts.priority = trafficBlks[i].priority;
                    rsegopts.outerColor = mbgl::Color(float(i) / float(trafficBlks.size() - 1), 0.0, 0.0, 1.0);
                    const bool success = rmptr_->routeSegmentCreate(routeID, rsegopts);
                    assert(success && "failed to create route segment");
                }
                trafficBlks.clear();
            }
            rmptr_->finalize();
        });

        success = true;
    }

    return success;
}

RouteAddTrafficTest::~RouteAddTrafficTest() {}
