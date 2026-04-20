
#include "route_test.hpp"

RouteTest::RouteTest(const std::string& testDir, const std::string& testName, mbgl::route::RouteManager* routeManager)
    : GLFWGraphicsTest(testName, testDir),
      rmptr_(routeManager) {
    assert(rmptr_ != nullptr && "RouteManager must not be null");
    if (!rmptr_) {
        throw std::invalid_argument("RouteManager pointer must not be null");
    }
}

bool RouteTest::initTestFixtures([[maybe_unused]] mbgl::Map* map) {
    assert(map != nullptr && "invalid map!");
    bool status = false;
    if (map != nullptr) {
        rmptr_->setStyle(map->getStyle());
        status = true;
    }

    return status;
}

bool RouteTest::teardownTestFixtures([[maybe_unused]] mbgl::Map* map) {
    for (const auto& iter : routeMap_) {
        const RouteID& routeID = iter.first;
        if (routeID.isValid()) {
            rmptr_->routeDispose(routeID);
        }
    }

    return true;
}

void RouteTest::setVanishingRouteID(const RouteID& id) {
    vanishingRouteID_ = id;
    rmptr_->setVanishingRouteID(vanishingRouteID_);
}

RouteID RouteTest::getVanishingRouteID() const {
    return vanishingRouteID_;
}

mbgl::Point<double> RouteTest::getPoint(const RouteID& routeID, double percent) const {
    assert(routeID.isValid() && "invalid route!");
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        return routeMap_.at(routeID).getPoint(percent);
    }

    return {};
}

int RouteTest::consumeTestCommand(mbgl::Map* map, GLFWView* view) {
    if (!testCommands_.empty()) {
        const auto& cmd = testCommands_.front();
        if (cmd) {
            cmd(map, view);
            testCommands_.pop();
        }
        return testCommands_.size();
    }

    return -1;
}

RouteTest::~RouteTest() {}
