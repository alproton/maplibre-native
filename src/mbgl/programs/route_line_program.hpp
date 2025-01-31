//
// Created by spalaniappan on 1/7/25.
//
#pragma once

#include <mbgl/programs/program.hpp>
//TODO: we only include line_program, because some of common uniform variables(like ratio, units_to_pixels). Lets move those to a common file later.
#include <mbgl/programs/line_program.hpp>
#include <mbgl/programs/attributes.hpp>
#include <mbgl/programs/uniforms.hpp>
#include <mbgl/renderer/layers/render_line_layer.hpp>
#include <mbgl/util/geometry.hpp>


namespace mbgl {

class RenderTile;
class TransformState;
class LinePatternPos;
class ImagePosition;

using RouteLineLayoutAttributes = TypeList<attributes::pos_normal, attributes::data<uint8_t, 4>>;

class RouteLineProgram : public Program<RouteLineProgram,
                                        shaders::BuiltIn::LineRouteProgram,
                                        gfx::PrimitiveType::Triangle,
                                        RouteLineLayoutAttributes,
                                        TypeList<uniforms::matrix, uniforms::ratio, uniforms::units_to_pixels, uniforms::device_pixel_ratio>,
                                        TypeList<>,
                                        style::RouteLinePaintProperties> {

    public:
        static constexpr std::string_view Name{"RouteLineProgram"};
        const std::string_view typeName() const noexcept override {return Name;}

        using Program::Program;


    /*
     * @param p vertex position
     * @param e extrude normal
     * @param round whether the vertex uses a round line cap
     * @param up whether the line normal points up or down
     * @param dir direction of the line cap (-1/0/1)
     */
    static LayoutVertex layoutVertex(
        Point<int16_t> p, Point<double> e, bool round, bool up, int8_t dir, int32_t linesofar = 0) {
        return LayoutVertex{
                {{static_cast<int16_t>((p.x * 2) | (round ? 1 : 0)), static_cast<int16_t>((p.y * 2) | (up ? 1 : 0))}},
                {{// add 128 to store a byte in an unsigned byte
                    static_cast<uint8_t>(util::clamp(::round(extrudeScale * e.x) + 128, 0., 255.)),
                    static_cast<uint8_t>(util::clamp(::round(extrudeScale * e.y) + 128, 0., 255.)),

                    // Encode the -1/0/1 direction value into the first two bits of .z
                    // of a_data. Combine it with the lower 6 bits of `linesofar`
                    // (shifted by 2 bites to make room for the direction value). The
                    // upper 8 bits of `linesofar` are placed in the `w` component.
                    // `linesofar` is scaled down by `LINE_DISTANCE_SCALE` so that we
                    // can store longer distances while sacrificing precision.

                    // Encode the -1/0/1 direction value into .zw coordinates of
                    // a_data, which is normally covered by linesofar, so we need to
                    // merge them. The z component's first bit, as well as the sign
                    // bit is reserved for the direction, so we need to shift the
                    // linesofar.
                    static_cast<uint8_t>(((dir == 0 ? 0 : (dir < 0 ? -1 : 1)) + 1) | ((linesofar & 0x3F) << 2)),
                    static_cast<uint8_t>(linesofar >> 6)}}};
    }

    /*
     * Scale the extrusion vector so that the normal length is this value.
     * Contains the "texture" normals (-1..1). This is distinct from the extrude
     * normals for line joins, because the x-value remains 0 for the texture
     * normal array, while the extrude normal actually moves the vertex to
     * create the acute/bevelled line join.
     */
    static const int8_t extrudeScale = 63;

    static LayoutUniformValues layoutUniformValues(const style::RouteLinePaintProperties::PossiblyEvaluated&,
                                                   const RenderTile&,
                                                   const TransformState&,
                                                   const std::array<float, 2>& pixelsToGLUnits,
                                                   float pixelRatio);

    using RouteLineLayoutVertex = RouteLineProgram::LayoutVertex;
    using RouteLineAttributes = RouteLineProgram::AttributeList;


};
}