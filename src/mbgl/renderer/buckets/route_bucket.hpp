#pragma once

#include <mbgl/programs/route_line_program.hpp>

#include <mbgl/renderer/bucket.hpp>
#include <mbgl/gfx/vertex_buffer.hpp>
#include <mbgl/gfx/index_buffer.hpp>
#include <mbgl/programs/segment.hpp>
#include <map>

namespace mbgl {

class RouteBucket final : public Bucket {

public:
    RouteBucket() = default;
    void upload(gfx::UploadPass&) override;

    bool hasData() const override;

    void addFeature(const GeometryTileFeature&,
                    const GeometryCollection&,
                    const mbgl::ImagePositions& patternPositions,
                    const PatternLayerMap&,
                    std::size_t,
                    const CanonicalTileID&) override;

    using VertexVector = gfx::VertexVector<RouteLineProgram::RouteLineLayoutVertex>;
    const std::shared_ptr<VertexVector> sharedVertices = std::make_shared<VertexVector>();
    VertexVector& vertices = *sharedVertices;

    using TriangleIndexVector = gfx::IndexVector<gfx::Triangles>;
    const std::shared_ptr<TriangleIndexVector> sharedTriangles = std::make_shared<TriangleIndexVector>();
    TriangleIndexVector& triangles = *sharedTriangles;

    SegmentVector<RouteLineProgram::RouteLineAttributes> segments;

    std::map<std::string, RouteLineProgram::Binders> paintPropertyBinders;
    bool getDirty() const;
    void setDirty(bool dirty);

private:
    bool dirty = true;
    void addGeometry(const GeometryCoordinates&);
};

}