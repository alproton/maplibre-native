

#include <valarray>
#include <mbgl/route/route.hpp>
#include <mbgl/util/math.hpp>

#include <mbgl/route/route_segment.hpp>

namespace mbgl {

namespace route {

Route::Route(const LineString<double>& geometry, double routeSegTransitionDist)
    : geometry_(geometry), routeSegTransitionDist_(routeSegTransitionDist) {
    for (size_t i = 1; i < geometry_.size(); ++i) {
        mbgl::Point<double> a = geometry_[i];
        mbgl::Point<double> b = geometry_[i - 1];
        double dist = std::abs(mbgl::util::dist<double>(a, b));
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
}

mbgl::LineString<double> Route::getGeometry() const {
    return geometry_;
}

std::map<double, mbgl::Color> Route::getRouteColorStops(const mbgl::Color& routeColor) const {
    std::map<double, mbgl::Color> gradients;
    // const double EPSILON = routeSegTransitionDist_;
    // if (progress_ == 0.0) {
        gradients[0.0] = routeColor;
        gradients[1.0] = routeColor;
    // } else {
    //     gradients[0.0] = progressColor_;
    //     gradients[progress_] = progressColor_;
    //     gradients[progress_ + EPSILON] = routeColor;
    //     gradients[1.0] = routeColor;
    // }

    return gradients;
}

std::map<double, mbgl::Color> Route::getRouteSegmentColorStops(const mbgl::Color& routeColor) {
    const double EPSILON = routeSegTransitionDist_;
    if (segments_.empty()) {
        return getRouteColorStops(routeColor);
    }

    if (segGradient_.empty()) {
        for (size_t i = 0; i < segments_.size(); ++i) {
            const auto& segNormalizedPos = segments_[i].getNormalizedPositions();
            const auto& segColor = segments_[i].getRouteSegmentOptions().color;

            // gradients need to start at 0.0 and end with 1.0.
            // each route segment is padded with route color for a small transition gradient.
            for (size_t j = 0; j < segNormalizedPos.size(); ++j) {
                const auto& segPos = segNormalizedPos[j];
                // if (segPos < progress_) {
                //     continue;
                // }

                if ((i == 0 && j == 0) || (i == segments_.size() - 1 && j == segNormalizedPos.size() - 1)) {
                    continue;
                }

                if (j == 0) {
                    segGradient_[segPos - EPSILON] = routeColor;
                }

                segGradient_[segPos] = segColor;

                if (j == segNormalizedPos.size() - 1) {
                    segGradient_[segPos + EPSILON] = routeColor;
                }
            }
        }

        const auto& firstSegNormalizedPos = segments_[0].getNormalizedPositions();
        const auto& lastSegNormalizedPos = segments_[segments_.size() - 1].getNormalizedPositions();
        const auto& firstSegColor = segments_[0].getRouteSegmentOptions().color;
        const auto& lastSegColor = segments_[segments_.size() - 1].getRouteSegmentOptions().color;

        assert(firstSegNormalizedPos[0] >= 0.0 && "normalized positions cannot be < 0.0");
        double firstNpos = firstSegNormalizedPos[0];
        if (firstNpos < EPSILON) {
            segGradient_[0.0] = firstSegColor;
        } else {
            segGradient_[0.0] = routeColor;

            segGradient_[firstNpos - EPSILON] = routeColor;
            segGradient_[firstNpos] = firstSegColor;
        }

        if (lastSegNormalizedPos[lastSegNormalizedPos.size() - 1] >= (1.0 - EPSILON)) {
            segGradient_[1.0] = lastSegColor;
        } else {
            double lastNpos = lastSegNormalizedPos[lastSegNormalizedPos.size() - 1];
            segGradient_[lastNpos] = lastSegColor;
            segGradient_[lastNpos + EPSILON] = routeColor;
            segGradient_[1.0] = routeColor;
        }
    }

    if (progress_ == 0.0) {
        return segGradient_;
    }

    return applyProgressOnGradient();
}

std::map<double, mbgl::Color> Route::applyProgressOnGradient() {
    assert(!segGradient_.empty() && "gradients for segments must not empty");
    const double EPSILON = routeSegTransitionDist_;
    std::map<double, mbgl::Color> gradients;
    // if (progress_ > 0.0) {
    //     gradients[0.0] = progressColor_;
    //     gradients[progress_] = progressColor_;
    // }

    bool foundProgressEnd = false;
    for (const auto& iter : segGradient_) {
        if (iter.first < progress_) {
            continue;
        }

        double segPos = iter.first;
        mbgl::Color segColor = iter.second;
        if (!foundProgressEnd) {
            gradients[progress_ + EPSILON] = segColor;
            foundProgressEnd = true;
        }
        gradients[segPos] = segColor;
    }

    return gradients;
}

void Route::routeSetProgress(const double t) {
    progress_ = t;
}

double Route::routeGetProgress() const {
    return progress_;
}

uint32_t Route::getNumRouteSegments() const {
    return static_cast<uint32_t>(segments_.size());
}

std::vector<double> Route::getRouteSegmentDistances() const {
    return segDistances_;
}

bool Route::routeSegmentsClear() {
    segments_.clear();

    return true;
}

Route& Route::operator=(Route& other) noexcept {
    if (this == &other) {
        return *this;
    }
    segDistances_ = other.segDistances_;
    segments_ = other.segments_;
    geometry_ = other.geometry_;
    totalDistance_ = other.totalDistance_;
    segGradient_ = other.segGradient_;
    progress_ = other.progress_;

    return *this;
}

} // namespace route
} // namespace mbgl