#pragma once

#include "route_test.hpp"

class RouteAddTest : public RouteTest {
public:
    RouteAddTest() = delete;
    RouteAddTest(const std::string& testDir);
    bool produceTestCommands(mbgl::Map* map) override;
    ~RouteAddTest() override;
};
