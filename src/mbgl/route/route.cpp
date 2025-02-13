

#include <mbgl/route/route.hpp>
#include <mbgl/util/math.hpp>

#include <mbgl/route/route_segment.hpp>

namespace mbgl {

namespace route {
    Route::Route(const LineString<double>& geometry) : geometry_(geometry) {
        for(size_t i = 1; i < geometry_.size(); ++i) {
            mbgl::Point<double> a = geometry_[i];
            mbgl::Point<double> b = geometry_[i-1];
            double dist = mbgl::util::dist<double>(a, b);
            segDistances_.push_back(dist);
            totalDistance_ += dist;
        }
    }

    double Route::getTotalDistance() const {
        return totalDistance_;
    }

    bool Route::hasRouteSegments() const {
        return !segments_.empty();
    }

    void Route::routeSegmentCreate(const RouteSegmentOptions& rsegopts) {
        RouteSegment routeSeg(rsegopts, geometry_, segDistances_, totalDistance_);
        segments_.push_back(routeSeg);

        gradientDirty_ = true;
    }

    mbgl::LineString<double> Route::getGeometry() const {
        return geometry_;
    }

    std::map<double, mbgl::Color> Route::getRouteSegmentColorStops(const mbgl::Color& routeColor) const {
        std::map<double, mbgl::Color> gradients;
        const double EPSILON = 0.00001;
        for(size_t i = 0; i < segments_.size(); ++i) {
            const auto& segNormalizedPos = segments_[i].getNormalizedPositions();
            const auto& segColor = segments_[i].getRouteSegmentOptions().color;

            //gradients need to start at 0.0 and end with 1.0.
            //each route segment is padded with route color for a small transition gradient.
            for(size_t j = 0; j < segNormalizedPos.size(); ++j) {
                const auto& segPos = segNormalizedPos[j];

                if((i == 0 && j == 0) || (i == segments_.size()-1 && j == segNormalizedPos.size()-1)) {
                    continue;
                }

                if(j == 0) {
                    double start = segPos-EPSILON < 0.0 ? 0.0 : segPos-EPSILON;
                    gradients[start] = routeColor;
                }

                gradients[segPos] = segColor;

                if(j == segNormalizedPos.size()-1) {
                    double end = segPos+EPSILON > 1.0 ? 1.0 : segPos+EPSILON;
                    gradients[end] = routeColor;
                }
            }
        }

        if(segments_.empty()) {
            gradients[0.0] = routeColor;
            gradients[1.0] = routeColor;
        }
        else {
            const auto& firstSegNormalizedPos = segments_[0].getNormalizedPositions();
            const auto& lastSegNormalizedPos = segments_[segments_.size()-1].getNormalizedPositions();
            const auto& firstSegColor = segments_[0].getRouteSegmentOptions().color;
            const auto& lastSegColor = segments_[segments_.size()-1].getRouteSegmentOptions().color;

            assert(firstSegNormalizedPos[0] >= 0.0 && "normalized positions cannot be < 0.0");
            if(firstSegNormalizedPos[0] < EPSILON) {
                gradients[0.0] = firstSegColor;
            } else {
                gradients[0.0] = routeColor;
                double firstNpos = firstSegNormalizedPos[0];
                gradients[firstNpos-EPSILON] = routeColor;
                gradients[firstNpos] = firstSegColor;
            }


            if(lastSegNormalizedPos[lastSegNormalizedPos.size()-1] >= (1.0 - EPSILON)) {
                gradients[1.0] = lastSegColor;
            } else {
                double lastNpos = lastSegNormalizedPos[lastSegNormalizedPos.size()-1];
                gradients[lastNpos] = lastSegColor;
                gradients[lastNpos+EPSILON] = routeColor;
                gradients[1.0] = routeColor;
            }
        }

        return gradients;
    }

    std::vector<double> Route::getRouteSegmentDistances() const {
        return segDistances_;
    }

    bool Route::clear() {
        segments_.clear();

        return true;
    }

    bool Route::getGradientDirty() const {
        return gradientDirty_;
    }

    void Route::validateGradientDirty() {
        gradientDirty_ = false;
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
        segDistances_ = other.segDistances_;
        segments_ = other.segments_;
        geometry_ = other.geometry_;
        gradientDirty_ = other.gradientDirty_;
        totalDistance_ = other.totalDistance_;

        return *this;
    }


}
}