#include "route_nav_circle_test.hpp"

#include "../glfw_view.hpp"
#include <mbgl/style/style.hpp>
#include "route_fixtures.hpp"
#include "../glfw_view.hpp"

RouteNavCircleTest::RouteNavCircleTest(const std::string testDir)
    : RouteTest(testDir, "route_nav_circle_test") {}

bool RouteNavCircleTest::produceTestCommands(mbgl::Map* map, GLFWView* view) {
    assert(map != nullptr && "invalid map!");
    assert(view != nullptr && "invalid view!");
    using namespace mbgl::route;
    using namespace route_fixtures;
    bool success = false;

    if (map != nullptr && view != nullptr) {
        testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
            map->setDebug(mbgl::MapDebugOptions::TileBorders);
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

            view->enablePuck(true);
            assert(routeID.isValid() && "invalid route ID!");
            if (routeID.isValid()) {
                double bearing = 0.0;
                mbgl::Point<double> location = rmptr_->getPoint(routeID, 0.0, Precision::Fine, &bearing);
                view->setPuckLocation(location.y, location.x, bearing); // Set puck at the center of the circle
            }
        });

        success = true;
    }

    testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
        const RouteID& routeID = routeMap_.begin()->first;
        assert(routeID.isValid() && "invalid route ID!");
        if (routeID.isValid()) {
            double bearing = 0.0;
            mbgl::Point<double> location = rmptr_->getPoint(routeID, 0.25, Precision::Fine, &bearing);
            view->setPuckLocation(location.y, location.x, bearing);
        }
    });

    testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
        const RouteID& routeID = routeMap_.begin()->first;
        assert(routeID.isValid() && "invalid route ID!");
        if (routeID.isValid()) {
            double bearing = 0.0;
            mbgl::Point<double> location = rmptr_->getPoint(routeID, 0.5, Precision::Fine, &bearing);
            view->setPuckLocation(location.y, location.x, bearing);
        }
    });

    testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
        const RouteID& routeID = routeMap_.begin()->first;
        assert(routeID.isValid() && "invalid route ID!");
        if (routeID.isValid()) {
            double bearing = 0.0;
            mbgl::Point<double> location = rmptr_->getPoint(routeID, 0.75, Precision::Fine, &bearing);
            view->setPuckLocation(location.y, location.x, bearing);
        }
    });

    testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
        const RouteID& routeID = routeMap_.begin()->first;
        assert(routeID.isValid() && "invalid route ID!");
        if (routeID.isValid()) {
            double bearing = 0.0;
            mbgl::Point<double> location = rmptr_->getPoint(routeID, 1, Precision::Fine, &bearing);
            view->setPuckLocation(location.y, location.x, bearing);
        }
    });

    return success;
}

RouteNavCircleTest::~RouteNavCircleTest() {}
