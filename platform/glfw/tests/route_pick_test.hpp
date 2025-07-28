#pragma once
#include "route_test.hpp"

class RoutePickTest : public RouteTest {
public:
    RoutePickTest() = delete;
    RoutePickTest(const std::string& testDir);
    bool produceTestCommands(mbgl::Map* map, GLFWView* view) override;
    ~RoutePickTest() override;

private:
    RouteID pickedRouteID;
    bool pickRoute(GLFWView* view, double x, double y);
};
