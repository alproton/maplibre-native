
#include "mbgl/programs/segment.hpp"

#include <mbgl/route/route_manager.hpp>
#include <mbgl/route/id_pool.hpp>
#include <mbgl/style/style.hpp>
#include <assert.h>

namespace mbgl {
namespace route {
RouteManager& RouteManager::getInstance() noexcept {
    static RouteManager instance;
    return instance;
}

RouteManager::RouteManager()
    : routeIDpool_(100) {}

void RouteManager::setStyle(style::Style& style) {
    style_ = &style;
}

RouteID RouteManager::routeCreate() {
    RouteID rid;
    bool success = routeIDpool_.CreateID((rid.id));
    if (success && rid.isValid()) {
        routeMap_[rid] = {};
    }

    return rid;
}

RouteSegmentID RouteManager::routeSegmentCreate(const RouteID& routeID, const RouteSegmentOptions& routeSegOpts) {
    RouteSegmentID rsegid;
    bool success = routeSegmentIDpool_.CreateID(rsegid.id);
    if(success && rsegid.isValid() && routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        routeMap_[routeID][rsegid] = RouteSegment(routeSegOpts);
    }

    return rsegid;
}

bool RouteManager::routeSegmentUpdate(const RouteID& routeID, const RouteSegmentID& routeSegmentID, RouteSegmentOptions& segmentOptions) {
    if(routeID.isValid() && routeSegmentID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        if(routeMap_[routeID].find(routeSegmentID) != routeMap_[routeID].end()) {
            routeMap_[routeID][routeSegmentID].update(segmentOptions);

            return true;
        }
    }

    return false;
}

bool RouteManager::routeSegmentDispose(const RouteID& routeID, const RouteSegmentID& routeSegmentID) {
    if(routeID.isValid() && routeSegmentID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        if(routeMap_[routeID].find(routeSegmentID) != routeMap_[routeID].end()) {
            routeMap_[routeID].erase(routeSegmentID);

            return true;
        }
    }

    return false;
}

bool RouteManager::routeDispose(const RouteID& routeID) {
    style_ = nullptr;
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        routeMap_.erase(routeID);

        return true;
    }

    return false;
}

void RouteManager::finalize() {
    assert(style_ != nullptr);
    if(style_ != nullptr) {

        finalized_ = true;
    }
}


RouteManager::~RouteManager() {}

}
}