#pragma once
#include "route_test.hpp"

class RouteCaptureTest : public RouteTest {
public:
    RouteCaptureTest() = delete;
    RouteCaptureTest(const std::string& testDir);
    bool produceTestCommands(mbgl::Map* map, GLFWView* view) override;
    ~RouteCaptureTest() override;

private:
    bool readAndLoadCapture(const std::string& capture_file_name, mbgl::Map* map);
};
