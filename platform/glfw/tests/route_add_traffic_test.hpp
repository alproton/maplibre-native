#pragma once

#include "route_test.hpp"
#include "route_fixtures.hpp"

class RouteAddTrafficTest : public RouteTest {
public:
    RouteAddTrafficTest() = delete;
    RouteAddTrafficTest(const std::string& testDir, mbgl::route::RouteManager* rm);
    bool produceTestCommands(mbgl::Map* map, GLFWView* view) override;
    ~RouteAddTrafficTest() override;
};
