#pragma once
#include <mbgl/renderer/bucket.hpp>
#include <mbgl/tile/geometry_tile_data.hpp>
#include <mbgl/gfx/vertex_buffer.hpp>
#include <mbgl/gfx/index_buffer.hpp>
#include <mbgl/programs/segment.hpp>
#include <mbgl/programs/line_program.hpp>
#include <mbgl/style/layers/line_layer_properties.hpp>
#include <mbgl/style/image_impl.hpp>

#include <vector>

namespace mbgl {

class BucketParameters;
class RenderLineLayer;

class LineBucket final : public Bucket {
public:
    using PossiblyEvaluatedLayoutProperties = style::LineLayoutProperties::PossiblyEvaluated;

    LineBucket(PossiblyEvaluatedLayoutProperties layout,
               const std::map<std::string, Immutable<style::LayerProperties>>& layerPaintProperties,
               float zoom,
               uint32_t overscaling);
    ~LineBucket() override;

    void addFeature(const GeometryTileFeature&,
                    const GeometryCollection&,
                    const mbgl::ImagePositions& patternPositions,
                    const PatternLayerMap&,
                    std::size_t,
                    const CanonicalTileID&) override;

    void setRouteBucket(bool isBucketForRoute) override { isRouteBucket = isBucketForRoute; }

    bool hasData() const override;

    void upload(gfx::UploadPass&) override;

    float getQueryRadius(const RenderLayer&) const override;
    double getPointIntersect(const Point<double>& pt);

    void update(const FeatureStates&, const GeometryTileLayer&, const std::string&, const ImagePositions&) override;

    PossiblyEvaluatedLayoutProperties layout;

    using VertexVector = gfx::VertexVector<LineLayoutVertex>;
    const std::shared_ptr<VertexVector> sharedVertices = std::make_shared<VertexVector>();
    VertexVector& vertices = *sharedVertices;

    using TriangleIndexVector = gfx::IndexVector<gfx::Triangles>;
    const std::shared_ptr<TriangleIndexVector> sharedTriangles = std::make_shared<TriangleIndexVector>();
    TriangleIndexVector& triangles = *sharedTriangles;
    SegmentVector<LineAttributes> segments;

#if MLN_LEGACY_RENDERER
    std::optional<gfx::VertexBuffer<LineLayoutVertex>> vertexBuffer;
    std::optional<gfx::IndexBuffer> indexBuffer;
#endif // MLN_LEGACY_RENDERER

    std::map<std::string, LineProgram::Binders> paintPropertyBinders;

private:
    struct RouteData {
        GeometryCoordinate lineCoordinate;
        float distanceSoFar = 0.0f;
    };
    void addGeometry(const GeometryCoordinates&, const GeometryTileFeature&, const CanonicalTileID&);
    double queryIntersection(const GeometryCoordinate& queryPt);

    CanonicalTileID canonicalTileID;

    const float zoom;
    const uint32_t overscaling;
    bool isRouteBucket = false;
    double totalLength_ = 0.0;
    // GeometryCoordinates lineCoordinates;
    // std::vector<float> lineCoordinatesDistanceSoFar;
    std::vector<RouteData> routeDataList_;
    std::vector<double> intervalLengths_;
    std::vector<double> cumulativeIntervalDistances_;
};

} // namespace mbgl
