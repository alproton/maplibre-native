
#include <mbgl/style/layer.hpp>
#include <mbgl/style/layers/line_layer.hpp>

#include <mbgl/route/route_manager.hpp>
#include <mbgl/route/id_pool.hpp>
#include <mbgl/style/style.hpp>
#include <assert.h>
#include <mapbox/geojsonvt.hpp>
#include <mbgl/route/route.hpp>
#include <mbgl/style/layers/line_layer.hpp>
#include <mbgl/style/sources/geojson_source.hpp>
#include <mbgl/style/expression/expression.hpp>
#include <mbgl/style/expression/literal.hpp>
#include <mbgl/style/expression/compound_expression.hpp>
#include <mbgl/style/expression/type.hpp>
#include <mbgl/style/expression/dsl.hpp>
#include <chrono>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>

namespace mbgl {
namespace route {

const std::string RouteManager::BASE_ROUTE_LAYER = "base_route_layer_";
const std::string RouteManager::ACTIVE_ROUTE_LAYER = "active_route_layer_";
const std::string RouteManager::GEOJSON_BASE_ROUTE_SOURCE_ID = "base_route_geojson_source_";
const std::string RouteManager::GEOJSON_ACTIVE_ROUTE_SOURCE_ID = "active_route_geojson_source_";
std::stringstream RouteManager::ss_;

namespace {

#define TRACE_FUNCTION_CALL(stream, stubstr) \
stream << __FUNCTION__ << std::endl << stubstr << std::endl;

std::string formatElapsedTime(long long value) {
    std::stringstream ss;
    ss.imbue(std::locale("")); // Use the user's default locale for number formatting
    ss << std::fixed << value;
    return ss.str();
}
}

RouteManager::RouteManager()
    : routeIDpool_(100) {}

void RouteManager::setStyle(style::Style& style) {
    if (style_ != nullptr && style_ != &style) {
        // remove the old base, active layer and source for each route and add them to the new style
        for (auto& routeIter : routeMap_) {
            const auto& routeID = routeIter.first;
            std::string baseLayerID = getBaseRouteLayerName(routeID);
            std::string activeLayerID = getActiveRouteLayerName(routeID);
            std::string baseGeoJSONsrcID = getBaseGeoJSONsourceName(routeID);
            std::string activeGeoJSONsrcID = getActiveGeoJSONsourceName(routeID);

            std::unique_ptr<style::Layer> baseLayer = style_->removeLayer(baseLayerID);
            std::unique_ptr<style::Source> baseGeoJSONsrc = style_->removeSource(baseGeoJSONsrcID);
            std::unique_ptr<style::Layer> activeLayer = style_->removeLayer(activeLayerID);
            std::unique_ptr<style::Source> activeGeoJSONsrc = style_->removeSource(activeGeoJSONsrcID);

            if (baseLayer) {
                style.addLayer(std::move(baseLayer));
            }
            if (baseGeoJSONsrc) {
                style.addSource(std::move(baseGeoJSONsrc));
            }
            if (activeLayer) {
                style.addLayer(std::move(activeLayer));
            }
            if (activeGeoJSONsrc) {
                style.addSource(std::move(activeGeoJSONsrc));
            }
            TRACE_FUNCTION_CALL(ss_, "transferred layers across styles");
        }
    }
    style_ = &style;
}

bool RouteManager::hasStyle() const {
    return style_ != nullptr;
}

RouteID RouteManager::routeCreate(const LineString<double>& geometry, const RouteOptions& ropts) {
    RouteID rid;
    bool success = routeIDpool_.createID((rid.id));
    if (success && rid.isValid()) {
        TRACE_FUNCTION_CALL(ss_, "created RouteID: "+std::to_string(rid.id));

        Route route(geometry, ropts);
        routeMap_[rid] = route;
        stats_.numRoutes++;
        dirtyRouteMap_[DirtyType::dtRouteGeometry].push_back(rid);
    }

    return rid;
}

void RouteManager::routeSegmentCreate(const RouteID& routeID, const RouteSegmentOptions& routeSegOpts) {
    if(routeID.isValid() && routeMap_.find(routeID) == routeMap_.end()) {
        TRACE_FUNCTION_CALL(ss_, "for routeID: "+std::to_string(routeID.id));

        routeMap_[routeID].routeSegmentCreate(routeSegOpts);
        stats_.numRouteSegments++;
        dirtyRouteMap_[DirtyType::dtRouteSegments].push_back(routeID);
    }
}

void RouteManager::routeClearSegments(const RouteID& routeID) {
    assert(routeID.isValid() && "invalid route ID");
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        stats_.numRouteSegments -= routeMap_[routeID].getNumRouteSegments();
        routeMap_[routeID].routeSegmentsClear();
        dirtyRouteMap_[DirtyType::dtRouteSegments].push_back(routeID);
    }
}

bool RouteManager::routeDispose(const RouteID& routeID) {
    assert(style_ != nullptr && "Style not set!");
    assert(routeID.isValid() && "Invalid route ID");
    assert(routeMap_.find(routeID) != routeMap_.end() && "Route not found internally");
    bool success = false;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end() && style_ != nullptr) {
        std::string baseLayerName = BASE_ROUTE_LAYER + std::to_string(routeID.id);
        std::string activeLayerName = ACTIVE_ROUTE_LAYER + std::to_string(routeID.id);
        std::string baseGeoJSONsrcName = GEOJSON_BASE_ROUTE_SOURCE_ID + std::to_string(routeID.id);
        std::string activeGeoJSONsrcName = GEOJSON_ACTIVE_ROUTE_SOURCE_ID + std::to_string(routeID.id);

        if (style_->removeLayer(baseLayerName) != nullptr) {
            success = true;
        }
        if (style_->removeLayer(activeLayerName) != nullptr) {
            success = true;
        }
        if (style_->removeSource(baseGeoJSONsrcName) != nullptr) {
            success = true;
        }

        if (style_->removeSource(activeGeoJSONsrcName) != nullptr) {
            success = true;
        }

        routeMap_.erase(routeID);
        stats_.numRoutes--;
        TRACE_FUNCTION_CALL(ss_, "disposed routeID: "+std::to_string(routeID.id));

        return success;
    }

    return success;
}

void RouteManager::setLayerBefore(const std::string& layerBefore) {
    layerBefore_ = layerBefore;
    TRACE_FUNCTION_CALL(ss_, "layerBefore: "+layerBefore_);

}

bool RouteManager::hasRoutes() const {
    return !routeMap_.empty();
}

bool RouteManager::routeSetProgress(const RouteID& routeID, const double progress) {
    assert(style_ != nullptr && "Style not set!");
    assert(routeID.isValid() && "invalid route ID");
    double validProgress = std::clamp(progress, 0.0, 1.0);
    bool success = false;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        TRACE_FUNCTION_CALL(ss_, "progress: "+std::to_string(validProgress));

        routeMap_[routeID].routeSetProgress(validProgress);
    }
    dirtyRouteMap_[DirtyType::dtRouteProgress].push_back(routeID);

    return success;
}

std::string RouteManager::getActiveRouteLayerName(const RouteID& routeID) const {
    return ACTIVE_ROUTE_LAYER + std::to_string(routeID.id);
}

std::string RouteManager::getBaseRouteLayerName(const RouteID& routeID) const {
    return BASE_ROUTE_LAYER + std::to_string(routeID.id);
}

std::string RouteManager::getActiveGeoJSONsourceName(const RouteID& routeID) const {
    return GEOJSON_ACTIVE_ROUTE_SOURCE_ID + std::to_string(routeID.id);
}

std::string RouteManager::getBaseGeoJSONsourceName(const RouteID& routeID) const {
    return GEOJSON_BASE_ROUTE_SOURCE_ID + std::to_string(routeID.id);
}

const std::string RouteManager::getStats() const {
    ss_<<"Num Routes: "<<stats_.numRoutes<<std::endl;
    ss_<<"Num finalized invocations: "<<stats_.numFinalizedInvoked<<std::endl;
    ss_<<"Num traffic zones: "<<stats_.numRouteSegments<<std::endl;
    ss_<<"InconsistentAPIusage: "<<std::boolalpha<<stats_.inconsistentAPIusage<<std::endl;
    ss_<<"Routes finilize elapsed: "<<stats_.finalizeMillis<<std::endl;

    return ss_.str();
}

void RouteManager::clearStats() {
    ss_.clear();
}

void RouteManager::finalizeRoute(const RouteID& routeID, const DirtyType& dt) {
    assert(routeID.isValid() && "invalid route ID");
    using namespace mbgl::style;
    using namespace mbgl::style::expression;

    const auto& createLayer = [&](const std::string& sourceID, const std::string& layerID, const Route& route, const Color& color, int width) {
        if (style_->getSource(sourceID) != nullptr) {
            return false;
        }
        mapbox::geojsonvt::feature_collection featureCollection;
        const auto& geom = route.getGeometry();
        featureCollection.emplace_back(geom);

        GeoJSONOptions opts;
        opts.lineMetrics = true;
        std::unique_ptr<GeoJSONSource> geoJSONsrc = std::make_unique<GeoJSONSource>(
            sourceID, mbgl::makeMutable<mbgl::style::GeoJSONOptions>(std::move(opts)));
        geoJSONsrc->setGeoJSON(featureCollection);

        style_->addSource(std::move(geoJSONsrc));

        // create the layers for each route
        std::unique_ptr<style::LineLayer> layer = std::make_unique<style::LineLayer>(layerID, sourceID);
        if (style_->getLayer(layerID) != nullptr) {
            return false;
        }
        layer->setLineColor(color);
        layer->setLineCap(LineCapType::Round);
        layer->setLineJoin(LineJoinType::Round);
        layer->setLineWidth(width);
        layer->setGradientLineFilter(LineGradientFilterType::Nearest);

        if (layerBefore_.empty()) {
            style_->addLayer(std::move(layer));
        } else {
            style_->addLayer(std::move(layer), layerBefore_);
        }

        return true;
    };

    const auto& createGradientExpression = [](const std::map<double, mbgl::Color>& gradient) {
        ParsingContext pc;
        ParseResult pr = createCompoundExpression("line-progress", {}, pc);
        std::unique_ptr<Expression> lineprogressValueExp = std::move(pr.value());

        Interpolator linearInterpolator = dsl::linear();

        using namespace mbgl::style::expression;
        std::map<double, std::unique_ptr<Expression>> stops;
        for (auto& segGradientIter : gradient) {
            stops[segGradientIter.first] = (dsl::literal(segGradientIter.second));
        }

        const auto& type = stops.begin()->second->getType();

        ParsingContext ctx;
        ParseResult result = createInterpolate(
            type, std::move(linearInterpolator), std::move(lineprogressValueExp), std::move(stops), ctx);
        assert(result);
        std::unique_ptr<Expression> expression = std::move(*result);

        return expression;
    };

    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {

        bool updateRouteLayers = false;
        bool updateGradients = false;
        bool updateProgress = false;
        switch (dt) {
            case DirtyType::dtRouteGeometry: {
                updateRouteLayers = true;
                updateGradients = true;
                updateProgress = true;
                TRACE_FUNCTION_CALL(ss_, "dirty route geometry");
            }
            break;

            case DirtyType::dtRouteProgress: {
                updateProgress = true;
                TRACE_FUNCTION_CALL(ss_, "dirty route progress");
            }
            break;

            case DirtyType::dtRouteSegments: {
                updateGradients = true;
                TRACE_FUNCTION_CALL(ss_, "dirty route segments");
            }
            break;

        }

        std::string baseLayerName = getBaseRouteLayerName(routeID);
        std::string baseGeoJSONSourceName = getBaseGeoJSONsourceName(routeID);
        std::string activeLayerName = getActiveRouteLayerName(routeID);
        std::string activeGeoJSONSourceName = getActiveGeoJSONsourceName(routeID);
        auto& route = routeMap_.at(routeID);
        const RouteOptions& routeOptions = route.getRouteOptions();

        if(updateRouteLayers) {
            //create layer for casing/base
            if(!createLayer(baseGeoJSONSourceName, baseLayerName, route, routeOptions.outerColor, routeOptions.outerWidth)) {
                stats_.inconsistentAPIusage = true;
            }

            //create layer for active/blue
            if(!createLayer(activeGeoJSONSourceName, activeLayerName, route, routeOptions.innerColor, routeOptions.innerWidth)) {
                stats_.inconsistentAPIusage = true;
            }
        }

        Layer* activeRouteLayer = style_->getLayer(activeLayerName);
        LineLayer* activeRouteLineLayer = static_cast<LineLayer*>(activeRouteLayer);
        Layer* baseRouteLayer = style_->getLayer(baseLayerName);
        LineLayer* baseRouteLineLayer = static_cast<LineLayer*>(baseRouteLayer);

        // Create the gradient colors expressions and set on the active layer
        if (updateGradients) {

            // create the gradient expression for active route.
            std::map<double, mbgl::Color> gradientMap = route.getRouteSegmentColorStops(
                routeOptions.innerColor);
            std::unique_ptr<expression::Expression> gradientExpression = createGradientExpression(
                gradientMap);


            ColorRampPropertyValue activeColorRampProp(std::move(gradientExpression));
            activeRouteLineLayer->setLineGradient(activeColorRampProp);

            // create the gradient expression for the base route
            std::map<double, mbgl::Color> baseLayerGradient = route.getRouteColorStops(routeOptions.outerColor);
            std::unique_ptr<expression::Expression> baseLayerExpression = createGradientExpression(
                baseLayerGradient);

            ColorRampPropertyValue baseColorRampProp(std::move(baseLayerExpression));
            baseRouteLineLayer->setLineGradient(baseColorRampProp);
        }

        if(updateProgress) {
            double progress = route.routeGetProgress();
            activeRouteLineLayer->setGradientLineClip(progress);
            baseRouteLineLayer->setGradientLineClip(progress);

        }
    }
}

void RouteManager::finalize() {
    using namespace mbgl::style;
    using namespace mbgl::style::expression;
    using namespace std::chrono;

    stats_.numFinalizedInvoked++;
    auto startclock = high_resolution_clock::now();
    {
        assert(style_ != nullptr);
        if (style_ != nullptr) {
            // create the layers and geojsonsource for base route
            for (const auto& iter : dirtyRouteMap_) {
                DirtyType dirtyType = iter.first;
                for(const auto& routeID : iter.second) {
                    finalizeRoute(routeID, dirtyType);
                }
            }
            dirtyRouteMap_.clear();
        }
    }
    auto stopclock = high_resolution_clock::now();
    stats_.finalizeMillis = formatElapsedTime(duration_cast<milliseconds>(stopclock - startclock).count());
}

RouteManager::~RouteManager() {}

} // namespace route
} // namespace mbgl