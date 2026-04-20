#pragma once

#include "route_test.hpp"

class RouteAddTest : public RouteTest {
public:
    RouteAddTest() = delete;
    RouteAddTest(const std::string& testDir, mbgl::route::RouteManager* rm);
    bool produceTestCommands(mbgl::Map* map, GLFWView* view) override;
    ~RouteAddTest() override;
};
