#pragma once

#include "route_test.hpp"

class RouteNavCircleTest : public RouteTest {
public:
    RouteNavCircleTest() = delete;
    RouteNavCircleTest(const std::string testDir, mbgl::route::RouteManager* rm);
    bool produceTestCommands(mbgl::Map* map, GLFWView* view) override;
    ~RouteNavCircleTest() override;
};
