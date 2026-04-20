#pragma once

#include <mbgl/style/layer_impl.hpp>
#include <mbgl/style/layers/line_layer.hpp>
#include <mbgl/style/layers/line_layer_properties.hpp>
#include <mbgl/util/color.hpp>
#include <vector>

namespace mbgl {
namespace style {

struct TrafficSegmentData {
    double start;
    double end;
    Color color;
};

class LineLayer::Impl : public Layer::Impl {
public:
    using Layer::Impl::Impl;

    bool hasLayoutDifference(const Layer::Impl&) const override;
    void stringifyLayout(rapidjson::Writer<rapidjson::StringBuffer>&) const override;

    expression::Dependency getDependencies() const noexcept override {
        return layout.getDependencies() | paint.getDependencies();
    }

    LineLayoutProperties::Unevaluated layout;
    LinePaintProperties::Transitionable paint;
    LineGradientFilterType gradientFilterType = LineGradientFilterType::Linear;
    bool isRouteLayer = false;
    std::vector<TrafficSegmentData> trafficSegments;
    bool useHighPrecisionTraffic = false;

    DECLARE_LAYER_TYPE_INFO;
};

} // namespace style
} // namespace mbgl
