#pragma once

#include <mbgl/map/mode.hpp>
#include <mbgl/tile/tile_id.hpp>
#include <mbgl/style/image_impl.hpp>
#include <mbgl/text/glyph.hpp>
#include <mbgl/actor/actor_ref.hpp>
#include <mbgl/util/immutable.hpp>
#include <mbgl/style/layer_properties.hpp>
#include <mbgl/geometry/feature_index.hpp>
#include <mbgl/renderer/bucket.hpp>
#include <mbgl/renderer/render_layer.hpp>
#include <mbgl/tile/tile.hpp>
#include <mbgl/util/containers.hpp>

#include <atomic>
#include <memory>

namespace mbgl {

class GeometryTile;
class GeometryTileData;
class Layout;

class LayoutResult {
public:
    mbgl::unordered_map<std::string, LayerRenderData> layerRenderData;
    std::shared_ptr<FeatureIndex> featureIndex;
    std::optional<AlphaImage> glyphAtlasImage;
    ImageAtlas iconAtlas;

    LayerRenderData* getLayerRenderData(const style::Layer::Impl&);

    LayoutResult(mbgl::unordered_map<std::string, LayerRenderData> renderData_,
                 std::unique_ptr<FeatureIndex> featureIndex_,
                 std::optional<AlphaImage> glyphAtlasImage_,
                 ImageAtlas iconAtlas_)
        : layerRenderData(std::move(renderData_)),
          featureIndex(std::move(featureIndex_)),
          glyphAtlasImage(std::move(glyphAtlasImage_)),
          iconAtlas(std::move(iconAtlas_)) {}
};

namespace style {
class Layer;
} // namespace style

class GeometryTileWorker {
public:
    GeometryTileWorker(ActorRef<GeometryTileWorker> self,
                       ActorRef<GeometryTile> parent,
                       const TaggedScheduler& scheduler_,
                       OverscaledTileID,
                       std::string,
                       const std::atomic<bool>&,
                       MapMode,
                       float pixelRatio,
                       bool showCollisionBoxes_);
    ~GeometryTileWorker();

    std::shared_ptr<LayoutResult> setLayers(std::vector<Immutable<style::LayerProperties>>,
                                            std::set<std::string> availableImages,
                                            uint64_t correlationID,
                                            bool sync = false);
    void setData(std::unique_ptr<const GeometryTileData>,
                 std::set<std::string> availableImages,
                 uint64_t correlationID);
    void reset(uint64_t correlationID_);
    void setShowCollisionBoxes(bool showCollisionBoxes_, uint64_t correlationID_);

    void onGlyphsAvailable(GlyphMap newGlyphMap);
    void onImagesAvailable(ImageMap newIconMap,
                           ImageMap newPatternMap,
                           ImageVersionMap versionMap,
                           uint64_t imageCorrelationID);

private:
    void coalesced();
    std::shared_ptr<LayoutResult> parse(bool ret = false);
    std::shared_ptr<LayoutResult> finalizeLayout(bool ret = false);

    void coalesce();

    void requestNewGlyphs(const GlyphDependencies&);
    void requestNewImages(const ImageDependencies&);

    void symbolDependenciesChanged();
    bool hasPendingDependencies() const;
    bool hasPendingParseResult() const;

    void checkPatternLayout(std::unique_ptr<Layout> layout);

    ActorRef<GeometryTileWorker> self;
    ActorRef<GeometryTile> parent;
    TaggedScheduler scheduler;

    const OverscaledTileID id;
    const std::string sourceID;
    const std::atomic<bool>& obsolete;
    const MapMode mode;
    const float pixelRatio;

    std::unique_ptr<FeatureIndex> featureIndex;
    mbgl::unordered_map<std::string, LayerRenderData> renderData;

    enum State {
        Idle,
        Coalescing,
        NeedsParse,
        NeedsSymbolLayout
    };

    State state = Idle;
    uint64_t correlationID = 0;
    uint64_t imageCorrelationID = 0;

    // Outer std::optional indicates whether we've received it or not.
    std::optional<std::vector<Immutable<style::LayerProperties>>> layers;
    std::optional<std::unique_ptr<const GeometryTileData>> data;

    std::vector<std::unique_ptr<Layout>> layouts;

    GlyphDependencies pendingGlyphDependencies;
    ImageDependencies pendingImageDependencies;
    GlyphMap glyphMap;
    ImageMap imageMap;
    ImageMap patternMap;
    ImageVersionMap versionMap;
    std::set<std::string> availableImages;

    bool showCollisionBoxes;
    bool firstLoad = true;
};

} // namespace mbgl
