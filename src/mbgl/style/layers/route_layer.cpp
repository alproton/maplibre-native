// clang-format off

// This file is generated. Edit scripts/generate-style-code.js, then run `make style-code`.

#include <mbgl/style/layers/route_layer.hpp>
#include <mbgl/style/layers/route_layer_impl.hpp>
#include <mbgl/style/layer_observer.hpp>
#include <mbgl/style/conversion/color_ramp_property_value.hpp>
#include <mbgl/style/conversion/constant.hpp>
#include <mbgl/style/conversion/property_value.hpp>
#include <mbgl/style/conversion/transition_options.hpp>
#include <mbgl/style/conversion/json.hpp>
#include <mbgl/style/conversion_impl.hpp>
#include <mbgl/util/traits.hpp>

#include <mapbox/eternal.hpp>

namespace mbgl {
namespace style {


// static
const LayerTypeInfo* RouteLayer::Impl::staticTypeInfo() noexcept {
    const static LayerTypeInfo typeInfo{"route",
                                        LayerTypeInfo::Source::NotRequired,
                                        LayerTypeInfo::Pass3D::NotRequired,
                                        LayerTypeInfo::Layout::NotRequired,
                                        LayerTypeInfo::FadingTiles::NotRequired,
                                        LayerTypeInfo::CrossTileIndex::NotRequired,
                                        LayerTypeInfo::TileKind::NotRequired};
    return &typeInfo;
}


RouteLayer::RouteLayer(const std::string& layerID, const std::string& sourceID)
    : Layer(makeMutable<Impl>(layerID, sourceID)) {
}

RouteLayer::RouteLayer(Immutable<Impl> impl_)
    : Layer(std::move(impl_)) {
}

RouteLayer::~RouteLayer() = default;

const RouteLayer::Impl& RouteLayer::impl() const {
    return static_cast<const Impl&>(*baseImpl);
}

Mutable<RouteLayer::Impl> RouteLayer::mutableImpl() const {
    return makeMutable<Impl>(impl());
}

std::unique_ptr<Layer> RouteLayer::cloneRef(const std::string& id_) const {
    auto impl_ = mutableImpl();
    impl_->id = id_;
    impl_->paint = LinePaintProperties::Transitionable();
    return std::make_unique<RouteLayer>(std::move(impl_));
}


void RouteLayer::Impl::stringifyLayout(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
    layout.stringify(writer);
}

// Layout properties

PropertyValue<LineCapType> RouteLayer::getDefaultLineCap() {
    return LineCap::defaultValue();
}

const PropertyValue<LineCapType>& RouteLayer::getLineCap() const {
    return impl().layout.get<LineCap>();
}

void RouteLayer::setLineCap(const PropertyValue<LineCapType>& value) {
    if (value == getLineCap()) return;
    auto impl_ = mutableImpl();
    impl_->layout.get<LineCap>() = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}
PropertyValue<LineJoinType> RouteLayer::getDefaultLineJoin() {
    return LineJoin::defaultValue();
}

const PropertyValue<LineJoinType>& RouteLayer::getLineJoin() const {
    return impl().layout.get<LineJoin>();
}

void RouteLayer::setLineJoin(const PropertyValue<LineJoinType>& value) {
    if (value == getLineJoin()) return;
    auto impl_ = mutableImpl();
    impl_->layout.get<LineJoin>() = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}
PropertyValue<float> RouteLayer::getDefaultLineMiterLimit() {
    return LineMiterLimit::defaultValue();
}

const PropertyValue<float>& RouteLayer::getLineMiterLimit() const {
    return impl().layout.get<LineMiterLimit>();
}

void RouteLayer::setLineMiterLimit(const PropertyValue<float>& value) {
    if (value == getLineMiterLimit()) return;
    auto impl_ = mutableImpl();
    impl_->layout.get<LineMiterLimit>() = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}
PropertyValue<float> RouteLayer::getDefaultLineRoundLimit() {
    return LineRoundLimit::defaultValue();
}

const PropertyValue<float>& RouteLayer::getLineRoundLimit() const {
    return impl().layout.get<LineRoundLimit>();
}

void RouteLayer::setLineRoundLimit(const PropertyValue<float>& value) {
    if (value == getLineRoundLimit()) return;
    auto impl_ = mutableImpl();
    impl_->layout.get<LineRoundLimit>() = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}
PropertyValue<float> RouteLayer::getDefaultLineSortKey() {
    return LineSortKey::defaultValue();
}

const PropertyValue<float>& RouteLayer::getLineSortKey() const {
    return impl().layout.get<LineSortKey>();
}

void RouteLayer::setLineSortKey(const PropertyValue<float>& value) {
    if (value == getLineSortKey()) return;
    auto impl_ = mutableImpl();
    impl_->layout.get<LineSortKey>() = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

// Paint properties

PropertyValue<float> RouteLayer::getDefaultLineBlur() {
    return {0.f};
}

const PropertyValue<float>& RouteLayer::getLineBlur() const {
    return impl().paint.template get<LineBlur>().value;
}

void RouteLayer::setLineBlur(const PropertyValue<float>& value) {
    if (value == getLineBlur())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineBlur>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineBlurTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineBlur>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineBlurTransition() const {
    return impl().paint.template get<LineBlur>().options;
}

PropertyValue<Color> RouteLayer::getDefaultLineColor() {
    return {Color::black()};
}

const PropertyValue<Color>& RouteLayer::getLineColor() const {
    return impl().paint.template get<LineColor>().value;
}

void RouteLayer::setLineColor(const PropertyValue<Color>& value) {
    if (value == getLineColor())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineColor>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineColorTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineColor>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineColorTransition() const {
    return impl().paint.template get<LineColor>().options;
}

PropertyValue<std::vector<float>> RouteLayer::getDefaultLineDasharray() {
    return {{}};
}

const PropertyValue<std::vector<float>>& RouteLayer::getLineDasharray() const {
    return impl().paint.template get<LineDasharray>().value;
}

void RouteLayer::setLineDasharray(const PropertyValue<std::vector<float>>& value) {
    if (value == getLineDasharray())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineDasharray>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineDasharrayTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineDasharray>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineDasharrayTransition() const {
    return impl().paint.template get<LineDasharray>().options;
}

PropertyValue<float> RouteLayer::getDefaultLineGapWidth() {
    return {0.f};
}

const PropertyValue<float>& RouteLayer::getLineGapWidth() const {
    return impl().paint.template get<LineGapWidth>().value;
}

void RouteLayer::setLineGapWidth(const PropertyValue<float>& value) {
    if (value == getLineGapWidth())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineGapWidth>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineGapWidthTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineGapWidth>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineGapWidthTransition() const {
    return impl().paint.template get<LineGapWidth>().options;
}

ColorRampPropertyValue RouteLayer::getDefaultLineGradient() {
    return {{}};
}

const ColorRampPropertyValue& RouteLayer::getLineGradient() const {
    return impl().paint.template get<LineGradient>().value;
}

void RouteLayer::setLineGradient(const ColorRampPropertyValue& value) {
    if (value == getLineGradient())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineGradient>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineGradientTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineGradient>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineGradientTransition() const {
    return impl().paint.template get<LineGradient>().options;
}

PropertyValue<float> RouteLayer::getDefaultLineOffset() {
    return {0.f};
}

const PropertyValue<float>& RouteLayer::getLineOffset() const {
    return impl().paint.template get<LineOffset>().value;
}

void RouteLayer::setLineOffset(const PropertyValue<float>& value) {
    if (value == getLineOffset())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineOffset>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineOffsetTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineOffset>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineOffsetTransition() const {
    return impl().paint.template get<LineOffset>().options;
}

PropertyValue<float> RouteLayer::getDefaultLineOpacity() {
    return {1.f};
}

const PropertyValue<float>& RouteLayer::getLineOpacity() const {
    return impl().paint.template get<LineOpacity>().value;
}

void RouteLayer::setLineOpacity(const PropertyValue<float>& value) {
    if (value == getLineOpacity())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineOpacity>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineOpacityTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineOpacity>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineOpacityTransition() const {
    return impl().paint.template get<LineOpacity>().options;
}

PropertyValue<expression::Image> RouteLayer::getDefaultLinePattern() {
    return {{}};
}

const PropertyValue<expression::Image>& RouteLayer::getLinePattern() const {
    return impl().paint.template get<LinePattern>().value;
}

void RouteLayer::setLinePattern(const PropertyValue<expression::Image>& value) {
    if (value == getLinePattern())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LinePattern>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLinePatternTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LinePattern>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLinePatternTransition() const {
    return impl().paint.template get<LinePattern>().options;
}

PropertyValue<std::array<float, 2>> RouteLayer::getDefaultLineTranslate() {
    return {{{0.f, 0.f}}};
}

const PropertyValue<std::array<float, 2>>& RouteLayer::getLineTranslate() const {
    return impl().paint.template get<LineTranslate>().value;
}

void RouteLayer::setLineTranslate(const PropertyValue<std::array<float, 2>>& value) {
    if (value == getLineTranslate())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineTranslate>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineTranslateTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineTranslate>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineTranslateTransition() const {
    return impl().paint.template get<LineTranslate>().options;
}

PropertyValue<TranslateAnchorType> RouteLayer::getDefaultLineTranslateAnchor() {
    return {TranslateAnchorType::Map};
}

const PropertyValue<TranslateAnchorType>& RouteLayer::getLineTranslateAnchor() const {
    return impl().paint.template get<LineTranslateAnchor>().value;
}

void RouteLayer::setLineTranslateAnchor(const PropertyValue<TranslateAnchorType>& value) {
    if (value == getLineTranslateAnchor())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineTranslateAnchor>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineTranslateAnchorTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineTranslateAnchor>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineTranslateAnchorTransition() const {
    return impl().paint.template get<LineTranslateAnchor>().options;
}

PropertyValue<float> RouteLayer::getDefaultLineWidth() {
    return {1.f};
}

const PropertyValue<float>& RouteLayer::getLineWidth() const {
    return impl().paint.template get<LineWidth>().value;
}

void RouteLayer::setLineWidth(const PropertyValue<float>& value) {
    if (value == getLineWidth())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineWidth>().value = value;
    impl_->paint.template get<LineFloorWidth>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void RouteLayer::setLineWidthTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<LineWidth>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions RouteLayer::getLineWidthTransition() const {
    return impl().paint.template get<LineWidth>().options;
}

using namespace conversion;

namespace {

constexpr uint8_t kPaintPropertyCount = 22u;

enum class Property : uint8_t {
    LineBlur,
    LineColor,
    LineDasharray,
    LineGapWidth,
    LineGradient,
    LineOffset,
    LineOpacity,
    LinePattern,
    LineTranslate,
    LineTranslateAnchor,
    LineWidth,
    LineBlurTransition,
    LineColorTransition,
    LineDasharrayTransition,
    LineGapWidthTransition,
    LineGradientTransition,
    LineOffsetTransition,
    LineOpacityTransition,
    LinePatternTransition,
    LineTranslateTransition,
    LineTranslateAnchorTransition,
    LineWidthTransition,
    LineCap = kPaintPropertyCount,
    LineJoin,
    LineMiterLimit,
    LineRoundLimit,
    LineSortKey,
};

template <typename T>
constexpr uint8_t toUint8(T t) noexcept {
    return uint8_t(mbgl::underlying_type(t));
}

constexpr const auto layerProperties = mapbox::eternal::hash_map<mapbox::eternal::string, uint8_t>(
    {{"line-blur", toUint8(Property::LineBlur)},
     {"line-color", toUint8(Property::LineColor)},
     {"line-dasharray", toUint8(Property::LineDasharray)},
     {"line-gap-width", toUint8(Property::LineGapWidth)},
     {"line-gradient", toUint8(Property::LineGradient)},
     {"line-offset", toUint8(Property::LineOffset)},
     {"line-opacity", toUint8(Property::LineOpacity)},
     {"line-pattern", toUint8(Property::LinePattern)},
     {"line-translate", toUint8(Property::LineTranslate)},
     {"line-translate-anchor", toUint8(Property::LineTranslateAnchor)},
     {"line-width", toUint8(Property::LineWidth)},
     {"line-blur-transition", toUint8(Property::LineBlurTransition)},
     {"line-color-transition", toUint8(Property::LineColorTransition)},
     {"line-dasharray-transition", toUint8(Property::LineDasharrayTransition)},
     {"line-gap-width-transition", toUint8(Property::LineGapWidthTransition)},
     {"line-gradient-transition", toUint8(Property::LineGradientTransition)},
     {"line-offset-transition", toUint8(Property::LineOffsetTransition)},
     {"line-opacity-transition", toUint8(Property::LineOpacityTransition)},
     {"line-pattern-transition", toUint8(Property::LinePatternTransition)},
     {"line-translate-transition", toUint8(Property::LineTranslateTransition)},
     {"line-translate-anchor-transition", toUint8(Property::LineTranslateAnchorTransition)},
     {"line-width-transition", toUint8(Property::LineWidthTransition)},
     {"line-cap", toUint8(Property::LineCap)},
     {"line-join", toUint8(Property::LineJoin)},
     {"line-miter-limit", toUint8(Property::LineMiterLimit)},
     {"line-round-limit", toUint8(Property::LineRoundLimit)},
     {"line-sort-key", toUint8(Property::LineSortKey)}});

StyleProperty getLayerProperty(const RouteLayer& layer, Property property) {
    switch (property) {
        case Property::LineBlur:
            return makeStyleProperty(layer.getLineBlur());
        case Property::LineColor:
            return makeStyleProperty(layer.getLineColor());
        case Property::LineDasharray:
            return makeStyleProperty(layer.getLineDasharray());
        case Property::LineGapWidth:
            return makeStyleProperty(layer.getLineGapWidth());
        case Property::LineGradient:
            return makeStyleProperty(layer.getLineGradient());
        case Property::LineOffset:
            return makeStyleProperty(layer.getLineOffset());
        case Property::LineOpacity:
            return makeStyleProperty(layer.getLineOpacity());
        case Property::LinePattern:
            return makeStyleProperty(layer.getLinePattern());
        case Property::LineTranslate:
            return makeStyleProperty(layer.getLineTranslate());
        case Property::LineTranslateAnchor:
            return makeStyleProperty(layer.getLineTranslateAnchor());
        case Property::LineWidth:
            return makeStyleProperty(layer.getLineWidth());
        case Property::LineBlurTransition:
            return makeStyleProperty(layer.getLineBlurTransition());
        case Property::LineColorTransition:
            return makeStyleProperty(layer.getLineColorTransition());
        case Property::LineDasharrayTransition:
            return makeStyleProperty(layer.getLineDasharrayTransition());
        case Property::LineGapWidthTransition:
            return makeStyleProperty(layer.getLineGapWidthTransition());
        case Property::LineGradientTransition:
            return makeStyleProperty(layer.getLineGradientTransition());
        case Property::LineOffsetTransition:
            return makeStyleProperty(layer.getLineOffsetTransition());
        case Property::LineOpacityTransition:
            return makeStyleProperty(layer.getLineOpacityTransition());
        case Property::LinePatternTransition:
            return makeStyleProperty(layer.getLinePatternTransition());
        case Property::LineTranslateTransition:
            return makeStyleProperty(layer.getLineTranslateTransition());
        case Property::LineTranslateAnchorTransition:
            return makeStyleProperty(layer.getLineTranslateAnchorTransition());
        case Property::LineWidthTransition:
            return makeStyleProperty(layer.getLineWidthTransition());
        case Property::LineCap:
            return makeStyleProperty(layer.getLineCap());
        case Property::LineJoin:
            return makeStyleProperty(layer.getLineJoin());
        case Property::LineMiterLimit:
            return makeStyleProperty(layer.getLineMiterLimit());
        case Property::LineRoundLimit:
            return makeStyleProperty(layer.getLineRoundLimit());
        case Property::LineSortKey:
            return makeStyleProperty(layer.getLineSortKey());
    }
    return {};
}

StyleProperty getLayerProperty(const RouteLayer& layer, const std::string& name) {
    const auto it = layerProperties.find(name.c_str());
    if (it == layerProperties.end()) {
        return {};
    }
    return getLayerProperty(layer, static_cast<Property>(it->second));
}

} // namespace

Value RouteLayer::serialize() const {
    auto result = Layer::serialize();
    assert(result.getObject());
    for (const auto& property : layerProperties) {
        auto styleProperty = getLayerProperty(*this, static_cast<Property>(property.second));
        if (styleProperty.getKind() == StyleProperty::Kind::Undefined) continue;
        serializeProperty(result, styleProperty, property.first.c_str(), property.second < kPaintPropertyCount);
    }
    return result;
}

std::optional<Error> RouteLayer::setPropertyInternal(const std::string& name, const Convertible& value) {
    const auto it = layerProperties.find(name.c_str());
    if (it == layerProperties.end()) return Error{"layer doesn't support this property"};

    auto property = static_cast<Property>(it->second);

    if (property == Property::LineBlur || property == Property::LineGapWidth || property == Property::LineOffset ||
        property == Property::LineOpacity || property == Property::LineWidth || property == Property::LineSortKey) {
        Error error;
        const auto& typedValue = convert<PropertyValue<float>>(value, error, true, false);
        if (!typedValue) {
            return error;
        }

        if (property == Property::LineBlur) {
            setLineBlur(*typedValue);
            return std::nullopt;
        }

        if (property == Property::LineGapWidth) {
            setLineGapWidth(*typedValue);
            return std::nullopt;
        }

        if (property == Property::LineOffset) {
            setLineOffset(*typedValue);
            return std::nullopt;
        }

        if (property == Property::LineOpacity) {
            setLineOpacity(*typedValue);
            return std::nullopt;
        }

        if (property == Property::LineWidth) {
            setLineWidth(*typedValue);
            return std::nullopt;
        }

        if (property == Property::LineSortKey) {
            setLineSortKey(*typedValue);
            return std::nullopt;
        }
    }
    if (property == Property::LineColor) {
        Error error;
        const auto& typedValue = convert<PropertyValue<Color>>(value, error, true, false);
        if (!typedValue) {
            return error;
        }

        setLineColor(*typedValue);
        return std::nullopt;
    }
    if (property == Property::LineDasharray) {
        Error error;
        const auto& typedValue = convert<PropertyValue<std::vector<float>>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setLineDasharray(*typedValue);
        return std::nullopt;
    }
    if (property == Property::LineGradient) {
        Error error;
        const auto& typedValue = convert<ColorRampPropertyValue>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setLineGradient(*typedValue);
        return std::nullopt;
    }
    if (property == Property::LinePattern) {
        Error error;
        const auto& typedValue = convert<PropertyValue<expression::Image>>(value, error, true, false);
        if (!typedValue) {
            return error;
        }

        setLinePattern(*typedValue);
        return std::nullopt;
    }
    if (property == Property::LineTranslate) {
        Error error;
        const auto& typedValue = convert<PropertyValue<std::array<float, 2>>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setLineTranslate(*typedValue);
        return std::nullopt;
    }
    if (property == Property::LineTranslateAnchor) {
        Error error;
        const auto& typedValue = convert<PropertyValue<TranslateAnchorType>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setLineTranslateAnchor(*typedValue);
        return std::nullopt;
    }
    if (property == Property::LineCap) {
        Error error;
        const auto& typedValue = convert<PropertyValue<LineCapType>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setLineCap(*typedValue);
        return std::nullopt;
    }
    if (property == Property::LineJoin) {
        Error error;
        const auto& typedValue = convert<PropertyValue<LineJoinType>>(value, error, true, false);
        if (!typedValue) {
            return error;
        }

        setLineJoin(*typedValue);
        return std::nullopt;
    }
    if (property == Property::LineMiterLimit || property == Property::LineRoundLimit) {
        Error error;
        const auto& typedValue = convert<PropertyValue<float>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        if (property == Property::LineMiterLimit) {
            setLineMiterLimit(*typedValue);
            return std::nullopt;
        }

        if (property == Property::LineRoundLimit) {
            setLineRoundLimit(*typedValue);
            return std::nullopt;
        }
    }

    Error error;
    std::optional<TransitionOptions> transition = convert<TransitionOptions>(value, error);
    if (!transition) {
        return error;
    }

    if (property == Property::LineBlurTransition) {
        setLineBlurTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineColorTransition) {
        setLineColorTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineDasharrayTransition) {
        setLineDasharrayTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineGapWidthTransition) {
        setLineGapWidthTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineGradientTransition) {
        setLineGradientTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineOffsetTransition) {
        setLineOffsetTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineOpacityTransition) {
        setLineOpacityTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LinePatternTransition) {
        setLinePatternTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineTranslateTransition) {
        setLineTranslateTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineTranslateAnchorTransition) {
        setLineTranslateAnchorTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::LineWidthTransition) {
        setLineWidthTransition(*transition);
        return std::nullopt;
    }

    return Error{"layer doesn't support this property"};
}

StyleProperty RouteLayer::getProperty(const std::string& name) const {
    return getLayerProperty(*this, name);
}

Mutable<Layer::Impl> RouteLayer::mutableBaseImpl() const {
    return staticMutableCast<Layer::Impl>(mutableImpl());
}

} // namespace style
} // namespace mbgl

// clang-format on
