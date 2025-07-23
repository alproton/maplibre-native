#pragma once

#include "glfw_graphics_test.hpp"
#include "route_fixtures.hpp"
#include <mbgl/route/id_types.hpp>
#include <mbgl/route/route_manager.hpp>
#include <unordered_map>

class RouteTest : public GLFWGraphicsTest {
public:
    RouteTest() = delete;
    RouteTest(const std::string& testDir, const std::string& testName);
    bool initTestFixtures(mbgl::Map* map) override;
    bool teardownTestFixtures(mbgl::Map* map) override;
    int consumeTestCommand(mbgl::Map* map) override;
    void setVanishingRouteID(const RouteID& id);
    RouteID getVanishingRouteID() const;
    mbgl::Point<double> getPoint(const RouteID& routeID, double percent) const;
    ~RouteTest() override;

protected:
    std::unique_ptr<mbgl::route::RouteManager> rmptr_;
    std::unordered_map<RouteID, route_fixtures::RouteData, IDHasher<RouteID>> routeMap_;
    RouteID vanishingRouteID_;
};
