#include "route_capture_test.hpp"
#include "route_fixtures.hpp"
#include "../glfw_view.hpp"
#include "rapidjson/document.h"
#include <fstream>

RouteCaptureTest::RouteCaptureTest(const std::string& testDir)
    : RouteTest(testDir, "route_capture_test") {}

bool RouteCaptureTest::readAndLoadCapture(const std::string& capture_file_name, mbgl::Map* map) {
    using namespace route_fixtures;
    teardownTestFixtures(map);
    vanishingRouteID_ = RouteID();
    rmptr_->setStyle(map->getStyle());

    std::ifstream jsonfile(capture_file_name);
    if (!jsonfile.is_open()) {
        mbgl::Log::Info(mbgl::Event::Route, "Failed to open the file.");
        return false;
    }

    std::stringstream buffer;
    buffer << jsonfile.rdbuf();

    bool loadedCapture = rmptr_->loadCapture(buffer.str());
    // store the route data
    if (loadedCapture) {
        const std::vector<RouteID>& routeIDs = rmptr_->getAllRoutes();
        for (const RouteID& routeID : routeIDs) {
            RouteData routeData;
            auto routeGeom = rmptr_->routeGetGeometry(routeID);
            if (routeGeom.has_value()) {
                routeData.points = routeGeom.value();
                routeData.resolution = routeData.points.size();
            }
            routeData.radius = 50;
            routeData.xlate = 0;
            routeMap_[routeID] = routeData;
        }
    }

    return loadedCapture;
}

bool RouteCaptureTest::produceTestCommands(mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
    assert(map != nullptr && "invalid map!");
    map->jumpTo(mbgl::CameraOptions()
                    .withCenter(mbgl::LatLng{37.514740, -121.939640})
                    .withZoom(6)
                    .withBearing(0.0)
                    .withPitch(0.0));
    return readAndLoadCapture("captures/yosemite_route_capture.json", map);
}

RouteCaptureTest::~RouteCaptureTest() {}
