
#include <mbgl/renderer/layers/route_layer_tweaker.hpp>


#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/drawable.hpp>
#include <mbgl/programs/route_line_program.hpp>
#include <mbgl/renderer/layer_group.hpp>
#include <mbgl/renderer/paint_parameters.hpp>
#include <mbgl/shaders/line_layer_ubo.hpp>
#include <mbgl/shaders/shader_program_base.hpp>
#include <mbgl/style/layers/route_layer_properties.hpp>
#include <mbgl/util/logging.hpp>

namespace mbgl {
using namespace style;
using namespace shaders;



template <typename Property>
auto RouteLayerTweaker::evaluate([[maybe_unused]] const PaintParameters& parameters) const {
    const auto& evaluated = static_cast<const RouteLayerProperties&>(*evaluatedProperties).evaluated;
    return evaluated.get<Property>().constantOr(Property::defaultValue());
}

void RouteLayerTweaker::execute(LayerGroupBase& layerGroup, const PaintParameters& parameters) {
    auto& context = parameters.context;
    const auto& evaluated = static_cast<const LineLayerProperties&>(*evaluatedProperties).evaluated;

    const auto& linePatternValue = evaluated.get<LinePattern>().constantOr(Faded<expression::Image>{"", ""});

    const auto zoom = static_cast<float>(parameters.state.getZoom());


    if (!evaluatedPropsUniformBuffer || propertiesUpdated) {
        const LineEvaluatedPropsUBO propsUBO{/*color =*/evaluate<LineColor>(parameters),
                                             /*blur =*/evaluate<LineBlur>(parameters),
                                             /*opacity =*/evaluate<LineOpacity>(parameters),
                                             /*gapwidth =*/evaluate<LineGapWidth>(parameters),
                                             /*offset =*/evaluate<LineOffset>(parameters),
                                             /*width =*/evaluate<LineWidth>(parameters),
                                             /*floorwidth =*/evaluate<LineFloorWidth>(parameters),
                                             LineExpressionMask::None,
                                             0};

        context.emplaceOrUpdateUniformBuffer(evaluatedPropsUniformBuffer, &propsUBO);
        propertiesUpdated = false;
    }
    auto& layerUniforms = layerGroup.mutableUniformBuffers();

    layerUniforms.set(idLineEvaluatedPropsUBO, evaluatedPropsUniformBuffer);


    visitLayerGroupDrawables(layerGroup, [&](gfx::Drawable& drawable) {
        const auto shader = drawable.getShader();
        if (!drawable.getTileID() || !shader || !checkTweakDrawable(drawable)) {
            return;
        }

        const UnwrappedTileID tileID = drawable.getTileID()->toUnwrapped();

        auto* binders = static_cast<RouteLineProgram::Binders*>(drawable.getBinders());
        const auto* tile = drawable.getRenderTile();
        if (!binders || !tile) {
            assert(false);
            return;
        }

        const auto& translation = evaluated.get<LineTranslate>();
        const auto anchor = evaluated.get<LineTranslateAnchor>();
        constexpr bool nearClipped = false;
        constexpr bool inViewportPixelUnits = false; // from RenderTile::translatedMatrix
        const auto matrix = getTileMatrix(
            tileID, parameters, translation, anchor, nearClipped, inViewportPixelUnits, drawable);

        auto& drawableUniforms = drawable.mutableUniformBuffers();
        const LineDrawableUBO drawableUBO = {
            /*matrix = */ util::cast<float>(matrix),
            /*ratio = */ 1.0f / tileID.pixelsToTileUnits(1.0f, static_cast<float>(zoom)),
            0,
            0,
            0};
        drawableUniforms.createOrUpdate(idLineDrawableUBO, &drawableUBO, context);

        const auto lineInterpolationUBO = LineInterpolationUBO{
            /*color_t =*/std::get<0>(binders->get<LineColor>()->interpolationFactor(zoom)),
            /*blur_t =*/std::get<0>(binders->get<LineBlur>()->interpolationFactor(zoom)),
            /*opacity_t =*/std::get<0>(binders->get<LineOpacity>()->interpolationFactor(zoom)),
            /*gapwidth_t =*/std::get<0>(binders->get<LineGapWidth>()->interpolationFactor(zoom)),
            /*offset_t =*/std::get<0>(binders->get<LineOffset>()->interpolationFactor(zoom)),
            /*width_t =*/std::get<0>(binders->get<LineWidth>()->interpolationFactor(zoom)),
            0,
            0};
        drawableUniforms.createOrUpdate(idLineInterpolationUBO, &lineInterpolationUBO, context);

        const auto lineRouteInterpolaionUBO = LineRouteInterpolationUBO {
            routeDistanceTraversed
            };
        drawableUniforms.createOrUpdate(idLineRouteUBO, &lineRouteInterpolaionUBO, context);
    });
}


void RouteLayerTweaker::setRouteDistanceTraversed(float t) {
    routeDistanceTraversed = t;
}

}