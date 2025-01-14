#include <mbgl/programs/route_line_program.hpp>
#include <mbgl/style/layers/line_layer_properties.hpp>
#include <mbgl/renderer/render_tile.hpp>
#include <mbgl/map/transform_state.hpp>
#include <mbgl/geometry/line_atlas.hpp>

namespace mbgl {

using namespace style;

template <class Values, class... Args>
Values makeValues(const style::RouteLinePaintProperties::PossiblyEvaluated& properties,
                  const RenderTile& tile,
                  const TransformState& state,
                  const std::array<float, 2>& pixelsToGLUnits,
                  const float pixelRatio,
                  Args&&... args) {
    return Values{uniforms::matrix::Value(tile.translatedMatrix(
                      properties.get<LineTranslate>(), properties.get<LineTranslateAnchor>(), state)),
                  uniforms::ratio::Value(1.0f / tile.id.pixelsToTileUnits(1.0f, static_cast<float>(state.getZoom()))),
                  uniforms::units_to_pixels::Value({{1.0f / pixelsToGLUnits[0], 1.0f / pixelsToGLUnits[1]}}),
                  uniforms::device_pixel_ratio::Value(pixelRatio),
                  std::forward<Args>(args)...};
}

RouteLineProgram::LayoutUniformValues RouteLineProgram::layoutUniformValues(const style::RouteLinePaintProperties::PossiblyEvaluated& properties,
                                               const RenderTile& tile,
                                               const TransformState& state,
                                               const std::array<float, 2>& pixelsToGLUnits,
                                               float pixelRatio){
    return makeValues<RouteLineProgram::LayoutUniformValues>(properties, tile, state, pixelsToGLUnits, pixelRatio);
}


}

