
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

namespace {

#define TRACE_FUNCTION_CALL(stream, stubstr) \
stream <<"Event "<<std::to_string(eventID_++)+": "<< __FUNCTION__ << std::endl << stubstr << std::endl << std::endl;

std::string formatElapsedTime(long long value) {
    std::stringstream ss;
    ss.imbue(std::locale("")); // Use the user's default locale for number formatting
    ss << std::fixed << value;
    return ss.str();
}

std::string toString(const LineString<double>& line) {
    std::stringstream ss;
    ss<<"{"<<std::endl;
    for(size_t i = 0; i < line.size(); i++) {
        ss<<"{"<<line[i].x<<", "<<line[i].y<<"},"<<std::endl;
    }
    ss<<"}"<<std::endl;
    ss<<std::endl;

    return ss.str();
}

std::string toString(const std::map<double, mbgl::Color>& gradient) {
    std::stringstream ss;
    ss<<"{"<<std::endl;
    for(const auto& iter : gradient) {
        ss<<"{ "<<iter.first<<", "<<"("<<iter.second.r<<", "<<iter.second.g<<", "<<iter.second.b<<", "<<iter.second.a<<") },"<<std::endl;
    }
    ss<<"}"<<std::endl;

    return ss.str();
}

}

RouteManager::RouteManager()
    : routeIDpool_(100) {
    }

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
            if(captureStream_) {
                TRACE_FUNCTION_CALL(captureStream_, "");
            }
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
        if(captureStream_) {
            TRACE_FUNCTION_CALL(captureStream_, "routeID: "+std::to_string(rid.id)+"\n"+toString(geometry));
        }

        Route route(geometry, ropts);
        routeMap_[rid] = route;
        stats_.numRoutes++;
        dirtyRouteMap_[DirtyType::dtRouteGeometry].insert(rid);
    }

    return rid;
}

void RouteManager::routeSegmentCreate(const RouteID& routeID, const RouteSegmentOptions& routeSegOpts) {
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        if(captureStream_) {
            TRACE_FUNCTION_CALL(captureStream_, "routeID: "+std::to_string(routeID.id)+"\n"+toString(routeSegOpts.geometry));
        }

        routeMap_[routeID].routeSegmentCreate(routeSegOpts);
        stats_.numRouteSegments++;

        validateAddToDirtyBin(routeID, DirtyType::dtRouteSegments);
    }
}

void RouteManager::validateAddToDirtyBin(const RouteID& routeID, const DirtyType& dirtyBin) {
    //check if this route is in the list of dirty geometry routes. if so, then no need to set dirty segments, as it will be created during finalize
    bool foundDirtyRoute = false;
    if(dirtyRouteMap_.find(DirtyType::dtRouteGeometry) != dirtyRouteMap_.end()) {
        const auto& dirtyRouteIDs = dirtyRouteMap_[DirtyType::dtRouteGeometry];
        if(dirtyRouteIDs.find(routeID) != dirtyRouteIDs.end()) {
            foundDirtyRoute = true;
        }
    }
    if(!foundDirtyRoute) {
        dirtyRouteMap_[dirtyBin].insert(routeID);
    }
}

void RouteManager::routeClearSegments(const RouteID& routeID) {
    assert(routeID.isValid() && "invalid route ID");
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        if(captureStream_) {
            TRACE_FUNCTION_CALL(captureStream_, "routeID: "+std::to_string(routeID.id));
        }
        stats_.numRouteSegments -= routeMap_[routeID].getNumRouteSegments();
        if(routeMap_[routeID].hasRouteSegments()) {
            routeMap_[routeID].routeSegmentsClear();
            validateAddToDirtyBin(routeID, DirtyType::dtRouteSegments);
        }
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
        if(captureStream_) {
            TRACE_FUNCTION_CALL(captureStream_, "routeID: "+std::to_string(routeID.id));
        }

        return success;
    }

    return success;
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
        if(captureStream_) {
            TRACE_FUNCTION_CALL(captureStream_, "progress: "+std::to_string(validProgress));
        }

        routeMap_[routeID].routeSetProgress(validProgress);

        validateAddToDirtyBin(routeID, DirtyType::dtRouteProgress);

        success = true;
    }

    return success;
}

bool RouteManager::routeSetProgress(const RouteID& routeID, const mbgl::Point<double>& progressPoint) {
    assert(routeID.isValid() && "invalid route ID");
    bool success = false;
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        if(captureStream_) {
            TRACE_FUNCTION_CALL(captureStream_, "progress point: "+std::to_string(progressPoint.x)+", "+std::to_string(progressPoint.y));
        }
        double progressPercent = routeMap_.at(routeID).getProgressPercent(progressPoint);

        routeMap_[routeID].routeSetProgress(progressPercent);
        validateAddToDirtyBin(routeID, DirtyType::dtRouteProgress);
        success = true;
    }

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

const std::string RouteManager::getStats()  {
    statsStream_<<"Num Routes: "<<stats_.numRoutes<<std::endl;
    statsStream_<<"Num finalized invocations: "<<stats_.numFinalizedInvoked<<std::endl;
    statsStream_<<"Num traffic zones: "<<stats_.numRouteSegments<<std::endl;
    statsStream_<<"InconsistentAPIusage: "<<std::boolalpha<<stats_.inconsistentAPIusage<<std::endl;
    statsStream_<<"Routes finilize elapsed: "<<stats_.finalizeMillis<<std::endl;

    return statsStream_.str();
}

void RouteManager::clearStats() {
    statsStream_.clear();
}

void RouteManager::beginCapture() {
    capturing_ = true;
    captureStream_.clear();
}

const std::string RouteManager::endCapture() {
    capturing_ = false;
    std::string retStr = captureStream_.str();
    captureStream_.clear();
    eventID_ = 0;

    return retStr;
}

void RouteManager::finalizeRoute(const RouteID& routeID, const DirtyType& dt) {
    assert(routeID.isValid() && "invalid route ID");
    using namespace mbgl::style;
    using namespace mbgl::style::expression;
    std::string captureStr;
    const auto& createLayer = [&](const std::string& sourceID, const std::string& layerID, const Route& route, const Color& color, const Color& clipColor, int width) {
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
        layer->setGradientLineClipColor(clipColor);

        auto& routeOpts = route.getRouteOptions();
        if (routeOpts.layerBefore.empty()) {
            style_->addLayer(std::move(layer));
        } else {
            style_->addLayer(std::move(layer), routeOpts.layerBefore);
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
                if(captureStream_) {
                    captureStr += "dirty route geometry";
                }
            }
            break;

            case DirtyType::dtRouteProgress: {
                updateProgress = true;
                if(captureStream_) {
                    captureStr += "dirty route progress";
                }
            }
            break;

            case DirtyType::dtRouteSegments: {
                updateGradients = true;
                if(captureStream_) {
                    captureStr += "dirty route segments";
                }
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
            if(!createLayer(baseGeoJSONSourceName, baseLayerName, route, routeOptions.outerColor, routeOptions.outerClipColor, routeOptions.outerWidth)) {
                stats_.inconsistentAPIusage = true;
            }

            //create layer for active/blue
            if(!createLayer(activeGeoJSONSourceName, activeLayerName, route, routeOptions.innerColor, routeOptions.innerClipColor, routeOptions.innerWidth)) {
                stats_.inconsistentAPIusage = true;
            }
        }

        Layer* activeRouteLayer = style_->getLayer(activeLayerName);
        LineLayer* activeRouteLineLayer = static_cast<LineLayer*>(activeRouteLayer);
        if(!activeRouteLineLayer) {
            stats_.inconsistentAPIusage = true;
        }
        Layer* baseRouteLayer = style_->getLayer(baseLayerName);
        LineLayer* baseRouteLineLayer = static_cast<LineLayer*>(baseRouteLayer);
        if(!baseRouteLineLayer) {
            stats_.inconsistentAPIusage = true;
        }

        // Create the gradient colors expressions and set on the active layer
        if (updateGradients) {

            // create the gradient expression for active route.
            std::map<double, mbgl::Color> gradientMap = route.getRouteSegmentColorStops(
                routeOptions.innerColor);
            if(captureStream_) {
                captureStr += "\nGradient Map:\n"+toString(gradientMap);
            }
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

        if(captureStream_) {
            TRACE_FUNCTION_CALL(captureStream_, "routeID: "+std::to_string(routeID.id)+"\n"+captureStr);
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