#pragma once

#include "mbgl/tile/tile_id.hpp"

#include <mbgl/util/geometry.hpp>
#include <mbgl/tile/geometry_tile_data.hpp>
#include <mbgl/style/types.hpp>
#include <mbgl/gfx/vertex_vector.hpp>
#include <mbgl/gfx/index_vector.hpp>
#include <mbgl/programs/segment.hpp>
#include <mbgl/util/math.hpp>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>
#include <functional>
#include <optional>

namespace mbgl {
namespace gfx {

class PolylineGeneratorDistances {
public:
    PolylineGeneratorDistances(double clipStart_, double clipEnd_, double total_)
        : clipStart(clipStart_),
          clipEnd(clipEnd_),
          total(total_) {}

    // Scale line distance from tile units to [0, 2^15).
    double scaleToMaxLineDistance(double tileDistance) const;
    double unscaledDistance(double tileDistance) const;
    double routeDistance(double routeDistanceSoFar, double totalRouteDistance) const;

public:
    double clipStart;
    double clipEnd;
    double total;
};

struct PolylineGeneratorOptions {
    FeatureType type{FeatureType::LineString};
    style::LineJoinType joinType{style::LineJoinType::Miter};
    float miterLimit{2.f};
    style::LineCapType beginCap{style::LineCapType::Butt};
    style::LineCapType endCap{style::LineCapType::Butt};
    float roundLimit{1.f};
    uint32_t overscaling{1};
    std::optional<PolylineGeneratorDistances> clipDistances;
    bool isRoutePath = false;
    double totalInMeters;
    CanonicalTileID canonicalTileID = CanonicalTileID(0, 0, 0);

    Point<double> tileCoordinatesToLatLng(const Point<int16_t>& p) const {
        // assert(canonicalTileID.z != (~0) && canonicalTileID.x != (~0) && canonicalTileID.y != (~0));
        const double size = util::EXTENT * std::pow(2, canonicalTileID.z);
        const double x0 = util::EXTENT * static_cast<double>(canonicalTileID.x);
        const double y0 = util::EXTENT * static_cast<double>(canonicalTileID.y);

        double y2 = 180 - (p.y + y0) * 360 / size;
        return Point<double>((p.x + x0) * 360 / size - 180, std::atan(std::exp(y2 * M_PI / 180)) * 360.0 / M_PI - 90.0);
    }

    double haversineDist(const GeometryCoordinate& g0, const GeometryCoordinate& g1) const {
        Point<double> p0 = tileCoordinatesToLatLng(g0);
        Point<double> p1 = tileCoordinatesToLatLng(g1);
        return util::haversineDist(p0, p1);
    }
};

template <class PolylineLayoutVertex, class PolylineSegment>
class PolylineGenerator {
public:
    using Vertices = gfx::VertexVector<PolylineLayoutVertex>;
    using Segments = std::vector<PolylineSegment>;
    using LayoutVertexFunc = std::function<PolylineLayoutVertex(
        Point<int16_t> p, Point<double> e, bool round, bool up, int8_t dir, float linesofar /*= 0*/)>;
    using CreateSegmentFunc = std::function<PolylineSegment(std::size_t vertexOffset, std::size_t indexOffset)>;
    using GetSegmentFunc = std::function<mbgl::SegmentBase&(PolylineSegment& segment)>;
    using Indexes = gfx::IndexVector<gfx::Triangles>;

public:
    PolylineGenerator(Vertices& polylineVertices,
                      LayoutVertexFunc layoutVertexFunc,
                      Segments& polylineSegments,
                      CreateSegmentFunc createSegmentFunc,
                      GetSegmentFunc getSegmentFunc,
                      Indexes& polylineIndexes);

    ~PolylineGenerator() = default;

    void generate(const GeometryCoordinates& coordinates, const PolylineGeneratorOptions& options);

private:
    struct TriangleElement;

    void addCurrentVertex(const GeometryCoordinate& currentCoordinate,
                          double& distance,
                          double& distanceInMeters,
                          const Point<double>& normal,
                          double endLeft,
                          double endRight,
                          bool round,
                          std::size_t startVertex,
                          std::vector<TriangleElement>& triangleStore,
                          std::optional<PolylineGeneratorDistances> lineDistances,
                          const PolylineGeneratorOptions& popts);
    void addPieSliceVertex(const GeometryCoordinate& currentVertex,
                           double distance,
                           double distanceInMeters,
                           const Point<double>& extrude,
                           bool lineTurnsLeft,
                           std::size_t startVertex,
                           std::vector<TriangleElement>& triangleStore,
                           std::optional<PolylineGeneratorDistances> lineDistances,
                           const PolylineGeneratorOptions& popts);

private:
    Vertices& vertices;
    LayoutVertexFunc layoutVertex;
    Segments& segments;
    CreateSegmentFunc createSegment;
    GetSegmentFunc getSegment;
    Indexes& indexes;
    const bool logVertices = true;

    std::ptrdiff_t e1;
    std::ptrdiff_t e2;
    std::ptrdiff_t e3;
};

} // namespace gfx
} // namespace mbgl
