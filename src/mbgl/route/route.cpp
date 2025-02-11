
#include <mbgl/route/route.hpp>
#include <mbgl/route/route_segment.hpp>

namespace mbgl {

namespace route {
    Route::Route(){}

    void Route::routeSegmentCreate(const RouteSegmentOptions& rsegopts) {
        segments_[rsegopts.name] = rsegopts;
    }

    mbgl::LineString<double> Route::getGeometry(const std::string& name) const {
        return segments_.at(name).getRouteSegmentOptions().geometry;
    }

    bool Route::clear() {
        segments_.clear();

        return true;
    }

    bool Route::getDirty() const {
        return dirty_;
    }

    Route& Route::operator=(Route& other) noexcept {
        if(this == &other) {
            return *this;
        }
        segments_ = other.segments_;

        return *this;
    }


}
}