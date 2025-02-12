
#include "mbgl/route/route_manager.hpp"

#include "mbgl/programs/segment.hpp"
#include "mbgl/style/layer.hpp"

#include <mbgl/route/route_manager.hpp>
#include <mbgl/route/id_pool.hpp>
#include <mbgl/style/style.hpp>
#include <assert.h>
#include <mapbox/geojsonvt.hpp>
#include <mbgl/route/route.hpp>
#include <mbgl/style/layers/line_layer.hpp>
#include <mbgl/style/sources/geojson_source.hpp>

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

RouteID RouteManager::routeCreate(const LineString<double>& geometry) {
    RouteID rid;
    bool success = routeIDpool_.CreateID((rid.id));
    if (success && rid.isValid()) {
        Route route(geometry);
        routeMap_[rid] = route;
    }

    return rid;
}

void RouteManager::routeSegmentCreate(const RouteID& routeID, const RouteSegmentOptions& routeSegOpts) {
     routeMap_[routeID].routeSegmentCreate(routeSegOpts);
}

bool RouteManager::routeDispose(const RouteID& routeID) {
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        //finalize will just rebuild the geojsonsource
        routeMap_.erase(routeID);
        return true;
    }

    return false;
}

void RouteManager::setLayerBefore(const std::string layerBefore) {
    layerBefore_ = layerBefore;
}

void RouteManager::setRouteCommonOptions(const RouteCommonOptions& ropts) {
    routeOptions_ = ropts;
}

void RouteManager::finalize() {
    using namespace mbgl::style;

    assert(style_ != nullptr);
    if(style_ != nullptr) {

        if(style_->getLayer(BASE_ROUTE_LAYER) == nullptr) {
            std::unique_ptr<style::LineLayer> layer0 = std::make_unique<style::LineLayer>(BASE_ROUTE_LAYER, GEOJSON_ROUTE_SOURCE_ID);
            layer0->setLineColor(routeOptions_.outerColor);
            layer0->setLineCap(LineCapType::Round);
            layer0->setLineJoin(LineJoinType::Round);
            layer0->setLineWidth(routeOptions_.outerWidth);

            if(layerBefore_.empty()) {
                style_->addLayer(std::move(layer0));
            } else {
                style_->addLayer(std::move(layer0), layerBefore_);
            }
        }

        if(style_->getLayer(ACTIVE_ROUTE_LAYER) == nullptr) {
            std::unique_ptr<style::LineLayer> layer1  = std::make_unique<style::LineLayer>(ACTIVE_ROUTE_LAYER, GEOJSON_ROUTE_SOURCE_ID);
            layer1->setLineColor(routeOptions_.innerColor);
            layer1->setLineCap(LineCapType::Round);
            layer1->setLineJoin(LineJoinType::Round);
            layer1->setLineWidth(routeOptions_.innerWidth);

            style_->addLayer(std::move(layer1));
        }

        mapbox::geojsonvt::feature_collection feature_collection;
        for(auto& iter : routeMap_) {
            const auto& route = iter.second;

            const auto& geom = route.getGeometry();
            // lines.push_back(geom);
            feature_collection.emplace_back(geom);
        }

        if(style_->getSource(GEOJSON_ROUTE_SOURCE_ID) != nullptr) {
            GeoJSONSource* geoJSONsrc = static_cast<GeoJSONSource*>(style_->getSource(GEOJSON_ROUTE_SOURCE_ID));
            geoJSONsrc->setGeoJSON(feature_collection);

        } else {
            GeoJSONOptions opts;
            opts.lineMetrics = true;
            std::unique_ptr<GeoJSONSource> geoJSONsrc = std::make_unique<GeoJSONSource>(GEOJSON_ROUTE_SOURCE_ID, mbgl::makeMutable<mbgl::style::GeoJSONOptions>(std::move(opts))) ;
            geoJSONsrc->setGeoJSON(feature_collection);

            style_->addSource(std::move(geoJSONsrc));
        }

        dirty_ = false;
    }
}


RouteManager::~RouteManager() {}

}
}