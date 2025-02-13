
// #include "mbgl/route/route_manager.hpp"

// #include "mbgl/programs/segment.hpp"
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

namespace mbgl {
namespace route {

const std::string RouteManager::BASE_ROUTE_LAYER = "base route layer";
const std::string RouteManager::ACTIVE_ROUTE_LAYER = "active route layer";
const std::string RouteManager::GEOJSON_ROUTE_SOURCE_ID = "route_geojson_source";

RouteManager& RouteManager::getInstance() noexcept {
    static RouteManager instance;
    return instance;
}

RouteManager::RouteManager()
    : routeIDpool_(100) {}

void RouteManager::setStyle(style::Style& style) {
    style_ = &style;
}

bool RouteManager::hasStyle() const {
    return style_ != nullptr;
}

RouteID RouteManager::routeCreate(const LineString<double>& geometry) {
    RouteID rid;
    bool success = routeIDpool_.CreateID((rid.id));
    if (success && rid.isValid()) {
        Route route(geometry);
        routeMap_[rid] = route;
        dirty_ = true;
    }

    return rid;
}

void RouteManager::routeSegmentCreate(const RouteID& routeID, const RouteSegmentOptions& routeSegOpts) {
     routeMap_[routeID].routeSegmentCreate(routeSegOpts);
    dirty_ = true;
}

bool RouteManager::routeDispose(const RouteID& routeID) {
    assert(style_ != nullptr && "Style not set!");
    assert(routeID.isValid() && "Invalid route ID");
    assert(routeMap_.find(routeID) != routeMap_.end() && "Route not found internally");
    bool success = false;
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end() && style_ != nullptr) {
        std::string baseLayerName = BASE_ROUTE_LAYER + std::to_string(routeID.id);
        std::string activeLayerName = ACTIVE_ROUTE_LAYER + std::to_string(routeID.id);
        std::string geoJSONsrcName = GEOJSON_ROUTE_SOURCE_ID + std::to_string(routeID.id);
        if(style_->removeLayer(baseLayerName) != nullptr) {
            success = true;
        }
        if(style_->removeLayer(activeLayerName) != nullptr) {
            success = true;
        }
        if(!style_->removeSource(geoJSONsrcName)) {
            success = true;
        }

        routeMap_.erase(routeID);
        return success;
    }

    return success;
}

void RouteManager::setLayerBefore(const std::string layerBefore) {
    layerBefore_ = layerBefore;
}

void RouteManager::setRouteCommonOptions(const RouteCommonOptions& ropts) {
    routeOptions_ = ropts;
}

bool RouteManager::hasRoutes() const {
    return !routeMap_.empty();
}

void RouteManager::finalize() {
    using namespace mbgl::style;
    using namespace mbgl::style::expression;
    assert(style_ != nullptr);
    if(style_ != nullptr && dirty_) {
        //create the layers and geojsonsource for base route
        for(const auto& iter : routeMap_) {
            std::string baseLayerName = BASE_ROUTE_LAYER + std::to_string(iter.first.id);
            std::string geoJSONSourceName = GEOJSON_ROUTE_SOURCE_ID + std::to_string(iter.first.id);
            const auto& route = iter.second;

            //create the layers for each route
            if(style_->getLayer(baseLayerName) == nullptr) {
                std::unique_ptr<style::LineLayer> baselayer = std::make_unique<style::LineLayer>(baseLayerName, geoJSONSourceName);
                baselayer->setLineColor(routeOptions_.outerColor);
                baselayer->setLineCap(LineCapType::Round);
                baselayer->setLineJoin(LineJoinType::Round);
                baselayer->setLineWidth(routeOptions_.outerWidth);

                if(layerBefore_.empty()) {
                    style_->addLayer(std::move(baselayer));
                } else {
                    style_->addLayer(std::move(baselayer), layerBefore_);
                }
            }

            if(style_->getSource(geoJSONSourceName) == nullptr) {

                mapbox::geojsonvt::feature_collection featureCollection;
                const auto& geom = route.getGeometry();
                featureCollection.emplace_back(geom);

                GeoJSONOptions opts;
                opts.lineMetrics = true;
                std::unique_ptr<GeoJSONSource> geoJSONsrc = std::make_unique<GeoJSONSource>(geoJSONSourceName, mbgl::makeMutable<mbgl::style::GeoJSONOptions>(std::move(opts))) ;
                geoJSONsrc->setGeoJSON(featureCollection);

                style_->addSource(std::move(geoJSONsrc));
            }
        }

        //create the layers, geojsonsource and gradients for active route
        // std::map<double, mbgl::Color> segmentGradients;
        for(auto& iter : routeMap_) {
            std::string activeLayerName = ACTIVE_ROUTE_LAYER + std::to_string(iter.first.id);
            std::string geoJSONSourceName = GEOJSON_ROUTE_SOURCE_ID + std::to_string(iter.first.id);
            auto& route = iter.second;

            //create the layers for each route
            if(style_->getLayer(activeLayerName) == nullptr) {
                std::unique_ptr<style::LineLayer> activelayer = std::make_unique<style::LineLayer>(activeLayerName, geoJSONSourceName);
                activelayer->setLineColor(routeOptions_.innerColor);
                activelayer->setLineCap(LineCapType::Round);
                activelayer->setLineJoin(LineJoinType::Round);
                activelayer->setLineWidth(routeOptions_.innerWidth);

                if(layerBefore_.empty()) {
                    style_->addLayer(std::move(activelayer));
                } else {
                    style_->addLayer(std::move(activelayer), layerBefore_);
                }
            }

            //create the geojson source for each route
            if(style_->getSource(geoJSONSourceName) == nullptr) {
                mapbox::geojsonvt::feature_collection featureCollection;
                const auto& geom = iter.second.getGeometry();
                featureCollection.emplace_back(geom);

                GeoJSONOptions opts;
                opts.lineMetrics = true;
                std::unique_ptr<GeoJSONSource> geoJSONsrc = std::make_unique<GeoJSONSource>(geoJSONSourceName, mbgl::makeMutable<mbgl::style::GeoJSONOptions>(std::move(opts))) ;
                geoJSONsrc->setGeoJSON(featureCollection);

                style_->addSource(std::move(geoJSONsrc));
            }

            //Create the gradient colors expressions and set on the active layer
            if(route.getGradientDirty()) {
                std::map<double, mbgl::Color> gradient = route.getRouteSegmentColorStops(routeOptions_.innerColor);

                ParsingContext pc;
                ParseResult pr = createCompoundExpression("line-progress", {}, pc);
                std::unique_ptr<Expression> lineprogressValueExp = std::move(pr.value());

                Interpolator linearInterpolator = dsl::linear();

                using namespace mbgl::style::expression;
                std::map<double, std::unique_ptr<Expression>> stops;
                for(auto& segGradientIter : gradient) {
                    stops[segGradientIter.first] = (dsl::literal(segGradientIter.second));
                }

                const auto& type = stops.begin()->second->getType();

                ParsingContext ctx;
                ParseResult result = createInterpolate(type, std::move(linearInterpolator), std::move(lineprogressValueExp), std::move(stops), ctx);
                assert(result);
                std::unique_ptr<Expression> expression = std::move(*result);

                ColorRampPropertyValue crpv(std::move(expression));
                Layer* activeRouteLayer = style_->getLayer(activeLayerName);
                LineLayer* activeRouteLineLayer = static_cast<LineLayer*>(activeRouteLayer);
                activeRouteLineLayer->setLineGradient(crpv);

                route.validateGradientDirty();
            }
        }

        dirty_ = false;
    }
}


RouteManager::~RouteManager() {}

}
}