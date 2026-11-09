#include <mbgl/test/util.hpp>
#include <mbgl/test/fake_file_source.hpp>
#include <mbgl/test/stub_tile_observer.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/tile/raster_tile.hpp>
#include <mbgl/tile/tile_loader_impl.hpp>

#include <mbgl/style/style.hpp>
#include <mbgl/util/run_loop.hpp>
#include <mbgl/map/transform.hpp>
#include <mbgl/annotation/annotation_manager.hpp>
#include <mbgl/renderer/tile_parameters.hpp>
#include <mbgl/renderer/buckets/raster_bucket.hpp>
#include <mbgl/renderer/image_manager.hpp>
#include <mbgl/text/glyph_manager.hpp>

using namespace mbgl;

class RasterTileTest {
public:
    util::SimpleIdentity uniqueID;
    std::shared_ptr<FileSource> fileSource = std::make_shared<FakeFileSource>();
    TransformState transformState;
    util::RunLoop loop;
    AnnotationManager annotationManager{style};
    std::shared_ptr<ImageManager> imageManager = std::make_shared<ImageManager>();
    std::shared_ptr<GlyphManager> glyphManager = std::make_shared<GlyphManager>();
    Tileset tileset{{"https://example.com"}, {0, 22}, "none"};
    TileParameters tileParameters;
    style::Style style;

    RasterTileTest()
        : tileParameters{1.0,
                         MapDebugOptions(),
                         transformState,
                         fileSource,
                         MapMode::Continuous,
                         annotationManager.makeWeakPtr(),
                         imageManager,
                         glyphManager,
                         0,
                         {Scheduler::GetBackground(), uniqueID}},
          style{fileSource, 1, tileParameters.threadPool} {}
};

TEST(RasterTile, ReloadFailedNetworkRequest) {
    RasterTileTest test;
    auto fileSource = std::static_pointer_cast<FakeFileSource>(test.fileSource);
    StubTileObserver observer;
    unsigned errors = 0;
    observer.tileError = [&](Tile&, std::exception_ptr) {
        ++errors;
    };
    observer.tileChanged = [&](Tile&) {
        test.loop.stop();
    };
    RasterTile tile(OverscaledTileID(0, 0, 0), "testSource", test.tileParameters, test.tileset, &observer);

    tile.reloadIfFailed();
    EXPECT_TRUE(fileSource->requests.empty());
    tile.setNecessity(TileNecessity::Required);
    ASSERT_EQ(1u, fileSource->requests.size());
    EXPECT_EQ(Resource::LoadingMethod::NetworkOnly, fileSource->requests.front()->resource.loadingMethod);

    Response failure;
    failure.error = std::make_unique<Response::Error>(Response::Error::Reason::Connection, "Connection failed");
    ASSERT_TRUE(fileSource->respond(Resource::Tile, failure));
    EXPECT_EQ(1u, errors);
    EXPECT_TRUE(tile.isLoaded());
    EXPECT_FALSE(tile.isRenderable());
    EXPECT_TRUE(fileSource->requests.empty());
    tile.setNecessity(TileNecessity::Required);
    EXPECT_TRUE(fileSource->requests.empty());

    tile.reloadIfFailed();
    ASSERT_EQ(1u, fileSource->requests.size());
    tile.reloadIfFailed();
    EXPECT_EQ(1u, fileSource->requests.size());

    Response success;
    success.data = std::make_shared<const std::string>(util::read_file("test/fixtures/image/tile.png"));
    ASSERT_TRUE(fileSource->respond(Resource::Tile, success));
    test.loop.run();
    EXPECT_TRUE(tile.isRenderable());
    EXPECT_TRUE(tile.isComplete());
    tile.reloadIfFailed();
    EXPECT_TRUE(fileSource->requests.empty());
    EXPECT_TRUE(tile.isRenderable());
    EXPECT_EQ(1u, errors);
}

TEST(RasterTile, ReloadOnlyFailedRequiredNetworkRequests) {
    RasterTileTest test;
    auto fileSource = std::static_pointer_cast<FakeFileSource>(test.fileSource);
    for (const auto reason : {Response::Error::Reason::Connection,
                              Response::Error::Reason::Server,
                              Response::Error::Reason::RateLimit,
                              Response::Error::Reason::Other}) {
        RasterTile tile(OverscaledTileID(0, 0, 0), "testSource", test.tileParameters, test.tileset);
        tile.setNecessity(TileNecessity::Required);
        Response failure;
        failure.error = std::make_unique<Response::Error>(reason, "Network request failed");
        ASSERT_TRUE(fileSource->respond(Resource::Tile, failure));
        tile.reloadIfFailed();
        ASSERT_EQ(1u, fileSource->requests.size());
        ASSERT_TRUE(fileSource->respond(Resource::Tile, failure));
        EXPECT_TRUE(fileSource->requests.empty());
        tile.setNecessity(TileNecessity::Optional);
        tile.reloadIfFailed();
        EXPECT_TRUE(fileSource->requests.empty());
    }
}

TEST(RasterTile, ReloadIgnoresCacheErrors) {
    class CachedFileSource : public FakeFileSource {
    public:
        bool supportsCacheOnlyRequests() const override { return true; }
    };
    RasterTileTest test;
    auto fileSource = std::make_shared<CachedFileSource>();
    test.tileParameters.fileSource = fileSource;
    RasterTile tile(OverscaledTileID(0, 0, 0), "testSource", test.tileParameters, test.tileset);
    ASSERT_EQ(1u, fileSource->requests.size());
    EXPECT_EQ(Resource::LoadingMethod::CacheOnly, fileSource->requests.front()->resource.loadingMethod);
    Response failure;
    failure.error = std::make_unique<Response::Error>(Response::Error::Reason::Connection, "Cache failed");
    ASSERT_TRUE(fileSource->respond(Resource::Tile, failure));
    tile.reloadIfFailed();
    EXPECT_TRUE(fileSource->requests.empty());

    tile.setNecessity(TileNecessity::Required);
    ASSERT_EQ(1u, fileSource->requests.size());
    tile.reloadIfFailed();
    EXPECT_EQ(1u, fileSource->requests.size());
}

TEST(RasterTile, ReloadIgnoresNotFound) {
    RasterTileTest test;
    auto fileSource = std::static_pointer_cast<FakeFileSource>(test.fileSource);
    StubTileObserver observer;
    observer.tileChanged = [&](Tile&) {
        test.loop.stop();
    };
    RasterTile tile(OverscaledTileID(0, 0, 0), "testSource", test.tileParameters, test.tileset, &observer);
    tile.setNecessity(TileNecessity::Required);
    Response missing;
    missing.error = std::make_unique<Response::Error>(Response::Error::Reason::NotFound, "Not found");
    missing.noContent = true;
    ASSERT_TRUE(fileSource->respond(Resource::Tile, missing));
    test.loop.run();
    tile.reloadIfFailed();
    EXPECT_TRUE(fileSource->requests.empty());
    EXPECT_FALSE(tile.isRenderable());
}

TEST(RasterTile, setError) {
    RasterTileTest test;
    RasterTile tile(OverscaledTileID(0, 0, 0), "testSource", test.tileParameters, test.tileset);
    tile.setError(std::make_exception_ptr(std::runtime_error("test")));
    EXPECT_FALSE(tile.isRenderable());
    EXPECT_TRUE(tile.isLoaded());
    EXPECT_TRUE(tile.isComplete());
}

TEST(RasterTile, onError) {
    RasterTileTest test;
    RasterTile tile(OverscaledTileID(0, 0, 0), "testSource", test.tileParameters, test.tileset);
    tile.onError(std::make_exception_ptr(std::runtime_error("test")), 0);
    EXPECT_FALSE(tile.isRenderable());
    EXPECT_TRUE(tile.isLoaded());
    EXPECT_TRUE(tile.isComplete());
}

TEST(RasterTile, onParsed) {
    RasterTileTest test;
    RasterTile tile(OverscaledTileID(0, 0, 0), "testSource", test.tileParameters, test.tileset);
    tile.onParsed(std::make_unique<RasterBucket>(PremultipliedImage{}), 0);
    EXPECT_TRUE(tile.isRenderable());
    EXPECT_TRUE(tile.isLoaded());
    EXPECT_TRUE(tile.isComplete());

    // Make sure that once we've had a renderable tile and then receive
    // erroneous data, we retain the previously rendered data and keep the tile
    // renderable.
    tile.setError(std::make_exception_ptr(std::runtime_error("Connection offline")));
    EXPECT_TRUE(tile.isRenderable());
    EXPECT_TRUE(tile.isLoaded());
    EXPECT_TRUE(tile.isComplete());

    // Then simulate a parsing failure and make sure that we keep it renderable
    // in this situation as well.
    tile.onError(std::make_exception_ptr(std::runtime_error("Parse error")), 0);
    ASSERT_TRUE(tile.isRenderable());
    EXPECT_TRUE(tile.isLoaded());
    EXPECT_TRUE(tile.isComplete());
}

TEST(RasterTile, onParsedEmpty) {
    RasterTileTest test;
    RasterTile tile(OverscaledTileID(0, 0, 0), "testSource", test.tileParameters, test.tileset);
    tile.onParsed(nullptr, 0);
    EXPECT_FALSE(tile.isRenderable());
    EXPECT_TRUE(tile.isLoaded());
    EXPECT_TRUE(tile.isComplete());
}
