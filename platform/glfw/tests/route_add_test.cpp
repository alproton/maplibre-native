#include "route_add_test.hpp"

#include <mbgl/style/style.hpp>
#include "route_fixtures.hpp"
#include "../glfw_view.hpp"

RouteAddTest::RouteAddTest(const std::string& testDir)
    : RouteTest("route_add_test", testDir) {}

bool RouteAddTest::produceTestCommands(mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
    assert(map != nullptr && "invalid map!");
    if (map != nullptr) {
        testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
            using namespace mbgl::route;
            using namespace route_fixtures;
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
            rd.resolution = 10;
            rd.xlate = 0.0;
            rd.radius = 50.0;
            rd.points = getRouteGeom(rd.radius, rd.resolution, rd.xlate);
            RouteOptions routeOpts;
            routeOpts.innerColor = color0;
            routeOpts.outerColor = mbgl::Color(0.2, 0.2, 0.2, 1);
            routeOpts.useDynamicWidths = false;
            routeOpts.outerClipColor = mbgl::Color(0.5, 0.5, 0.5, 1.0);
            routeOpts.innerClipColor = mbgl::Color(0.5, 0.5, 0.5, 1.0);
            routeOpts.useMercatorProjection = false;

            const RouteID routeID = rmptr_->routeCreate(rd.points, routeOpts);
            assert(routeID.isValid() && "invalid route ID created");
            rmptr_->finalize();

            setVanishingRouteID(routeID);
            routeMap_[routeID] = rd;
        });

        return true;
    }

    return false;
}

RouteAddTest::~RouteAddTest() {}
