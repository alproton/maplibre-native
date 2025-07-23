#pragma once

#include "route_test.hpp"

class RouteTrafficPriorityTest : public RouteTest {
    RouteTrafficPriorityTest() = delete;
    RouteTrafficPriorityTest(const std::string& testDir);
    bool produceTestCommands(mbgl::Map* map) override;
    ~RouteTrafficPriorityTest() override;
};
