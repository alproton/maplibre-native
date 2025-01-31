
#include <mbgl/renderer/buckets/route_bucket.hpp>
#include <mbgl/gfx/polyline_generator.hpp>

namespace mbgl {

void RouteBucket::upload([[maybe_unused]] gfx::UploadPass& uploadPass) {
    uploaded = true;
}

bool RouteBucket::hasData() const {
    return !segments.empty();
}

bool RouteBucket::getDirty() const {
    return dirty;
}

void RouteBucket::setDirty(bool dirtyFlag) {
    dirty = dirtyFlag;
}


void RouteBucket::addFeature(const GeometryTileFeature&,
                            const GeometryCollection& geometryCollection,
                            const ImagePositions& ,
                            const PatternLayerMap&,
                            std::size_t,
                            const CanonicalTileID&) {
    for (auto& line : geometryCollection) {
        addGeometry(line);
    }

    // for (auto& pair : paintPropertyBinders) {
    //     const auto it = patternDependencies.find(pair.first);
    //     if (it != patternDependencies.end()) {
    //         pair.second.populateVertexVectors(
    //             feature, vertices.elements(), index, patternPositions, it->second, canonical);
    //     } else {
    //         pair.second.populateVertexVectors(feature, vertices.elements(), index, patternPositions, {}, canonical);
    //     }
    // }
}

void RouteBucket::addGeometry(const GeometryCoordinates& /*coordinates*/) {
    // gfx::PolylineGenerator<RouteLineProgram::RouteLineLayoutVertex, Segment<RouteLineProgram::RouteLineAttributes>> generator(
    //     vertices,
    //     RouteLineProgram::layoutVertex,
    //     segments,
    //     [](std::size_t vertexOffset, std::size_t indexOffset) -> Segment<RouteLineProgram::RouteLineAttributes> {
    //         return Segment<RouteLineProgram::RouteLineAttributes>(vertexOffset, indexOffset);
    //     },
    //     [](auto& seg) -> Segment<RouteLineProgram::RouteLineAttributes>& { return seg; },
    //     triangles);
    //
    // //for now keep as much default
    // gfx::PolylineGeneratorOptions options;
    //
    // options.type = FeatureType::LineString;
    //
    // const std::size_t len = [&coordinates] {
    //     std::size_t l = coordinates.size();
    //     // If the line has duplicate vertices at the end, adjust length to remove them.
    //     while (l >= 2 && coordinates[l - 1] == coordinates[l - 2]) {
    //         l--;
    //     }
    //     return l;
    // }();
    //
    // const std::size_t first = [&coordinates, &len] {
    //     std::size_t i = 0;
    //     // If the line has duplicate vertices at the start, adjust index to remove them.
    //     while (i < len - 1 && coordinates[i] == coordinates[i + 1]) {
    //         i++;
    //     }
    //     return i;
    // }();
    //
    // // Ignore invalid geometry.
    // if (len < (options.type == FeatureType::Polygon ? 3 : 2)) {
    //     return;
    // }
    //
    // generator.generate(coordinates, options);

}



}