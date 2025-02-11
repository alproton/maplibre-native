
#include <mbgl/route/route.hpp>
#include <mbgl/route/route_segment.hpp>

namespace mbgl {

namespace route {
    Route::Route(const LineString<double>& geometry) : geometry_(geometry){

    }

    void Route::routeSegmentCreate(const RouteSegmentOptions& rsegopts) {
        RouteSegment rseg(rsegopts);
        segments_.push_back(rseg);
    }

    mbgl::LineString<double> Route::getGeometry() const {
        return geometry_;
    }

    bool Route::clear() {
        segments_.clear();

        return true;
    }

    bool Route::getDirty() const {
        return dirty_;
    }

    void Route::sortRouteSegments() {
        std::sort(segments_.begin(), segments_.end(), [](RouteSegment& a, RouteSegment& b) {
            return a.getSortOrder() < b.getSortOrder();
        });
    }

    Route& Route::operator=(Route& other) noexcept {
        if(this == &other) {
            return *this;
        }
        segments_ = other.segments_;
        geometry_ = other.geometry_;
        dirty_ = other.dirty_;
        return *this;
    }


}
}