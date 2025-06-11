#include "example_custom_drawable_style_layer.hpp"

#if MLN_DRAWABLE_RENDERER

#include <mbgl/style/layer.hpp>
#include <mbgl/style/layers/custom_drawable_layer.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/gfx/drawable.hpp>
#include <mbgl/util/logging.hpp>

#include <memory>
#include <cmath>
#include <filesystem>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

ExampleCustomDrawableStyleLayerHost::ExampleCustomDrawableStyleLayerHost(const std::string& assetsPath_)
    : assetsPath(assetsPath_) {}

ExampleCustomDrawableStyleLayerHost::~ExampleCustomDrawableStyleLayerHost() {}

void ExampleCustomDrawableStyleLayerHost::initialize() {}

void ExampleCustomDrawableStyleLayerHost::deinitialize() {}

void ExampleCustomDrawableStyleLayerHost::update(Interface& interface) {
    // if we have built our drawable(s) already, either update or skip
    if (interface.getDrawableCount() == 0) {
        createDrawables(interface);
        return;
    }
}

mbgl::Point<double> ExampleCustomDrawableStyleLayerHost::project(const mbgl::LatLng& c, const mbgl::TransformState& s) {
    mbgl::LatLng unwrappedLatLng = c.wrapped();
    unwrappedLatLng.unwrapForShortestPath(s.getLatLng(mbgl::LatLng::Wrapped));
    return mbgl::Projection::project(unwrappedLatLng, s.getScale());
}

void ExampleCustomDrawableStyleLayerHost::createDrawables(Interface& interface) {
    constexpr float extent = mbgl::util::EXTENT;

    // add classic polylines
    {
        using namespace mbgl;

        // set tile
        interface.setTileID({11, 327, 792});

        constexpr auto numLines = 1;
        Interface::LineOptions options[numLines]{
            {/*geometry=*/{},
             /*blur=*/0.0f,
             /*opacity=*/1.0f,
             /*gapWidth=*/0.0f,
             /*offset=*/0.0f,
             /*width=*/8.0f,
             /*color=*/Color::red()},

        };

        for (auto& opt : options) {
            opt.geometry.beginCap = style::LineCapType::Butt;
            opt.geometry.endCap = style::LineCapType::Butt;
            opt.geometry.joinType = style::LineJoinType::Miter;
        }

        constexpr auto numPoints = 10;
        GeometryCoordinates polyline;
        for (auto ipoint{0}; ipoint < numPoints; ++ipoint) {
            polyline.emplace_back(
                static_cast<int16_t>(ipoint * extent / numPoints),
                static_cast<int16_t>(std::sin(ipoint * 2 * M_PI / numPoints) * extent / numLines / 2.f));
        }

        for (auto index{0}; index < numLines; ++index) {
            for (auto& p : polyline) {
                p.y += extent / numLines;
            }

            // set property values
            interface.setLineOptions(options[index]);

            // add polyline
            interface.addPolyline(polyline, Interface::LineShaderType::Classic);
        }
    }

    // add polylines using wide vectors using geographic coordinates
    {
        using namespace mbgl;

        // add polyline with geographic coordinates
        Interface::LineOptions options = {/*geometry=*/{},
                                          /*blur=*/0.0f,
                                          /*opacity=*/1.0f,
                                          /*gapWidth=*/0.0f,
                                          /*offset=*/0.0f,
                                          /*width=*/12.0f,
                                          /*color=*/{.0f, 1.0f, .0f, 1.0f}};

        options.geometry.beginCap = style::LineCapType::Square;
        options.geometry.endCap = style::LineCapType::Square;
        options.geometry.joinType = style::LineJoinType::Bevel;
        options.geometry.type = FeatureType::LineString;
        interface.setLineOptions(options);

        LineString<double> polyline_geo{
            // San Francisco
            {-122.38186800073211, 37.77466003457463},
            {-122.3869373450997, 37.774352128895615},
            {-122.38680767979824, 37.773294612880306},
            {-122.38476465260224, 37.773350946288765},
            {-122.38146223043374, 37.77194168964067},
            {-122.6813560305925, 37.666084247570964},
            {-122.26765538866474, 37.65037232584494},
            {-122.42528413673159, 38.020443518012584},
            // Seattle
            {-122.34927775216809, 47.62050663596438},
            // New York
            {-74.04454331829972, 40.6892168305434},
        };
        interface.addPolyline(polyline_geo, Interface::LineShaderType::Classic);
    }

    // finish
    interface.finish();
}

void ExampleCustomDrawableStyleLayerHost::generateGeometry(Interface& interface) {
    constexpr float itemScale = 10.0f;
    const mbgl::LatLng location{37.78, -122.47};

    Interface::GeometryOptions options;

    // load image
    std::shared_ptr<mbgl::PremultipliedImage> image = std::make_shared<mbgl::PremultipliedImage>(
        mbgl::decodeImage(mbgl::util::read_file(assetsPath + "puck.png")));

    options.texture = interface.context.createTexture2D();
    options.texture->setImage(image);
    options.texture->setSamplerConfiguration(
        {mbgl::gfx::TextureFilterType::Linear, mbgl::gfx::TextureWrapType::Clamp, mbgl::gfx::TextureWrapType::Clamp});

    const std::shared_ptr<VertexVector> sharedVertices = std::make_shared<VertexVector>();
    VertexVector& vertices = *sharedVertices;

    const std::shared_ptr<TriangleIndexVector> sharedIndices = std::make_shared<TriangleIndexVector>();
    TriangleIndexVector& indices = *sharedIndices;

    const unsigned long numVtxCircumference = 72;
    const float bearingStep = 360.0f / static_cast<float>(numVtxCircumference - 1);

    vertices.emplace_back(Interface::GeometryVertex{{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}});

    for (unsigned long i = 1; i <= numVtxCircumference; ++i) {
        const float rad = mbgl::util::deg2radf((i - 1) * bearingStep);

        Interface::GeometryVertex vertex;
        vertex.position = {sinf(rad) / 2.0f, cosf(rad) / 2.0f, 0.0f};
        vertex.texcoords = {0.5f + vertex.position[0], 0.5f - vertex.position[1]};

        vertices.emplace_back(std::move(vertex));
    }

    for (uint16_t i = 1; i < vertices.elements() - 1; ++i) {
        indices.emplace_back(0, i, static_cast<uint16_t>(i + 1));
    }
    indices.emplace_back(0, static_cast<uint16_t>(vertices.elements() - 1), 1);

    interface.setGeometryOptions(options);

    interface.setGeometryTweakerCallback([=, frameCount = 0]([[maybe_unused]] mbgl::gfx::Drawable& drawable,
                                                             const mbgl::PaintParameters& params,
                                                             Interface::GeometryOptions& currentOptions) mutable {
        frameCount++;

        const mbgl::Point<double>& center = project(location, params.state);

        const float scale = itemScale * static_cast<float>(std::pow(2.f, params.state.getZoom())) *
                            params.pixelsToGLUnits[0];

        mbgl::mat4 matrix = mbgl::matrix::identity4();
        mbgl::matrix::translate(matrix, matrix, center.x, center.y, 0.0);
        mbgl::matrix::rotate_z(matrix, matrix, frameCount * 0.05f);
        mbgl::matrix::scale(matrix, matrix, scale, scale, 1.0f);
        mbgl::matrix::multiply(currentOptions.matrix, params.transformParams.nearClippedProjMatrix, matrix);

        if (frameCount % 100 == 0) {
            if (currentOptions.color.g > 0.0f) {
                currentOptions.color = mbgl::Color::red();
            } else {
                currentOptions.color = mbgl::Color::white();
            }
        }
    });

    interface.addGeometry(sharedVertices, sharedIndices, false);
}

void ExampleCustomDrawableStyleLayerHost::loadGeometry(Interface& interface) {
    constexpr float itemScale = 0.1f;
    constexpr std::array<float, 3> itemRotation = {90.0f, 0.0f, 0.0f};
    const mbgl::LatLng location{37.76, -122.47};

    Interface::GeometryOptions options;

    interface.setGeometryTweakerCallback([=]([[maybe_unused]] mbgl::gfx::Drawable& drawable,
                                             const mbgl::PaintParameters& params,
                                             Interface::GeometryOptions& currentOptions) mutable {
        const mbgl::Point<double>& center = project(location, params.state);

        const float scale = itemScale * static_cast<float>(std::pow(2.f, params.state.getZoom())) *
                            params.pixelsToGLUnits[0];

        mbgl::mat4 matrix = mbgl::matrix::identity4();
        mbgl::matrix::translate(matrix, matrix, center.x, center.y, 0.0);
        mbgl::matrix::rotate_x(matrix, matrix, mbgl::util::deg2radf(itemRotation[0]));
        mbgl::matrix::rotate_y(matrix, matrix, mbgl::util::deg2radf(itemRotation[1]));
        mbgl::matrix::rotate_z(matrix, matrix, mbgl::util::deg2radf(itemRotation[2]));
        mbgl::matrix::scale(matrix, matrix, scale, scale, scale);
        mbgl::matrix::multiply(currentOptions.matrix, params.transformParams.nearClippedProjMatrix, matrix);
    });

    const std::shared_ptr<VertexVector> sharedVertices = std::make_shared<VertexVector>();
    const std::shared_ptr<TriangleIndexVector> sharedIndices = std::make_shared<TriangleIndexVector>();

    // importObj(interface, "../../_deps/tinyobjloader-src/models/cube.obj", *sharedVertices, *sharedIndices, options);
    importObj(interface, assetsPath + "sphere.obj", *sharedVertices, *sharedIndices, options);

    interface.setGeometryOptions(options);
    interface.addGeometry(sharedVertices, sharedIndices, true);
}

void ExampleCustomDrawableStyleLayerHost::importObj(Interface& interface,
                                                    const std::string& filename,
                                                    VertexVector& vertices,
                                                    TriangleIndexVector& indices,
                                                    Interface::GeometryOptions& options) {
    tinyobj::ObjReaderConfig readerConfig;

    readerConfig.triangulate = true;
    readerConfig.vertex_color = false;

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename, readerConfig)) {
        if (!reader.Error().empty()) {
            mbgl::Log::Error(mbgl::Event::General, reader.Error());
        }

        assert(false);
        throw std::runtime_error(std::string("Cannot read obj file ") + filename);
    }

    if (!reader.Warning().empty()) {
        mbgl::Log::Error(mbgl::Event::General, reader.Warning());
    }

    const tinyobj::attrib_t& attributes = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();

    if (attributes.texcoords.empty()) {
        options.color = mbgl::Color::green();
    } else {
        // TODO parse material

        options.texture = createCheckerboardTexture(interface, 32, 32, 40, {255, 255, 255, 255}, {0, 0, 0, 255});
    }

    for (const auto& shape : shapes) {
        const auto shapeIndexStart = static_cast<uint16_t>(indices.elements());

        for (uint16_t face = 0; face < shape.mesh.indices.size(); face += 3) {
            for (size_t i = 0; i < 3; ++i) {
                const tinyobj::index_t index = shape.mesh.indices[face + i];

                Interface::GeometryVertex vertex = {};

                std::copy(attributes.vertices.begin() + vertex.position.size() * index.vertex_index,
                          attributes.vertices.begin() + vertex.position.size() * (index.vertex_index + 1),
                          vertex.position.begin());

                if (!attributes.texcoords.empty()) {
                    std::copy(attributes.texcoords.begin() + vertex.texcoords.size() * index.texcoord_index,
                              attributes.texcoords.begin() + vertex.texcoords.size() * (index.texcoord_index + 1),
                              vertex.texcoords.begin());
                }

                vertices.emplace_back(std::move(vertex));
            }

            indices.emplace_back(shapeIndexStart + face, shapeIndexStart + face + 1, shapeIndexStart + face + 2);
        }
    }
}

mbgl::gfx::Texture2DPtr ExampleCustomDrawableStyleLayerHost::createCheckerboardTexture(
    Interface& interface,
    uint16_t wb,
    uint16_t hb,
    uint16_t blockSize,
    const std::array<uint8_t, 4>& color1,
    const std::array<uint8_t, 4>& color2) {
    std::shared_ptr<mbgl::PremultipliedImage> image = std::make_shared<mbgl::PremultipliedImage>(
        mbgl::Size(wb * blockSize, hb * blockSize));

    constexpr uint8_t pixelSize = sizeof(uint8_t) * 4;

    for (uint16_t row = 0; row < hb; ++row) {
        for (uint16_t blockRow = 0; blockRow < blockSize; ++blockRow) {
            for (uint16_t col = 0; col < wb; ++col) {
                for (uint16_t blockCol = 0; blockCol < blockSize; ++blockCol) {
                    auto index = (row * blockSize + blockRow) * image->stride() +
                                 (col * blockSize + blockCol) * pixelSize;

                    memcpy(image->data.get() + index, (row + col) % 2 == 0 ? color1.data() : color2.data(), pixelSize);
                }
            }
        }
    }

    auto texture = interface.context.createTexture2D();
    texture->setSamplerConfiguration(
        {mbgl::gfx::TextureFilterType::Linear, mbgl::gfx::TextureWrapType::Clamp, mbgl::gfx::TextureWrapType::Clamp});
    texture->setImage(std::move(image));

    return texture;
}

#endif
