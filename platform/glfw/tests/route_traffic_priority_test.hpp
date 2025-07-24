#pragma once

#include "route_test.hpp"

class RouteTrafficPriorityTest : public RouteTest {
public:
    RouteTrafficPriorityTest() = delete;
    RouteTrafficPriorityTest(const std::string& testDir);
    bool produceTestCommands(mbgl::Map* map, GLFWView* view) override;
    ~RouteTrafficPriorityTest() override;

private:
    enum RouteSegmentTestCases {
        Blk1LowPriorityIntersecting,
        Blk1HighPriorityIntersecting,
        Blk12SameColorIntersecting,
        Blk12NonIntersecting,
        Invalid
    };

    void setTrafficCase(const RouteSegmentTestCases& testcase) const;
};
