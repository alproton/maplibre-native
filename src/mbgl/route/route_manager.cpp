
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
#include <mbgl/style/layers/location_indicator_layer.hpp>
#include <mbgl/util/io.hpp>

namespace mbgl {
namespace route {

const std::string RouteManager::BASE_ROUTE_LAYER = "base_route_layer_";
const std::string RouteManager::ACTIVE_ROUTE_LAYER = "active_route_layer_";
const std::string RouteManager::GEOJSON_BASE_ROUTE_SOURCE_ID = "base_route_geojson_source_";
const std::string RouteManager::GEOJSON_ACTIVE_ROUTE_SOURCE_ID = "active_route_geojson_source_";
const std::string RouteManager::PUCK_LAYER_ID = "puck_layer";

std::stringstream RouteManager::ss_;

RouteManager::RouteManager()
    : routeIDpool_(100) {}

void RouteManager::setStyle(style::Style& style) {
    if(style_ != nullptr && style_ != &style) {
        //remove the old base, active layer and source for each route and add them to the new style
        for(auto& routeIter : routeMap_) {
            const auto& routeID = routeIter.first;
            std::string baseLayerID = getBaseRouteLayerName(routeID);
            std::string activeLayerID = getActiveRouteLayerName(routeID);
            std::string baseGeoJSONsrcID = getBaseGeoJSONsourceName(routeID);
            std::string activeGeoJSONsrcID = getActiveGeoJSONsourceName(routeID);

            std::unique_ptr<style::Layer> baseLayer = style_->removeLayer(baseLayerID);
            std::unique_ptr<style::Source> baseGeoJSONsrc = style_->removeSource(baseGeoJSONsrcID);
            std::unique_ptr<style::Layer> activeLayer = style_->removeLayer(activeLayerID);
            std::unique_ptr<style::Source>  activeGeoJSONsrc = style_->removeSource(activeGeoJSONsrcID);

            if(baseLayer) {
                style.addLayer(std::move(baseLayer));
            }
            if(baseGeoJSONsrc) {
                style.addSource(std::move(baseGeoJSONsrc));
            }
            if(activeLayer) {
                style.addLayer(std::move(activeLayer));
            }
            if(activeGeoJSONsrc) {
                style.addSource(std::move(activeGeoJSONsrc));
            }
        }
    }
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
        stats_.numRouteSegments++;
        dirty_ = true;
    }

    return rid;
}

void RouteManager::routeSegmentCreate(const RouteID& routeID, const RouteSegmentOptions& routeSegOpts) {
     routeMap_[routeID].routeSegmentCreate(routeSegOpts);
     stats_.numRouteSegments++;
    dirty_ = true;
}

void RouteManager::routeClearSegments(const RouteID& routeID) {
    assert(routeID.isValid() && "invalid route ID");
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        stats_.numRouteSegments -= routeMap_[routeID].getNumRouteSegments();
        routeMap_[routeID].routeSegmentsClear();
    }

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
        std::string baseGeoJSONsrcName = GEOJSON_BASE_ROUTE_SOURCE_ID + std::to_string(routeID.id);
        std::string activeGeoJSONsrcName = GEOJSON_ACTIVE_ROUTE_SOURCE_ID + std::to_string(routeID.id);

        if(style_->removeLayer(baseLayerName) != nullptr) {
            success = true;
        }
        if(style_->removeLayer(activeLayerName) != nullptr) {
            success = true;
        }
        if(style_->removeSource(baseGeoJSONsrcName) != nullptr) {
            success = true;
        }

        if(style_->removeSource(activeGeoJSONsrcName) != nullptr) {
            success = true;
        }

        routeMap_.erase(routeID);
        stats_.numRoutes--;
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

bool RouteManager::routeSetProgress(const RouteID& routeID, const double progress) {
    assert(style_ != nullptr && "Style not set!");
    assert(routeID.isValid() && "invalid route ID");
    double validProgress = std::clamp(progress, 0.0, 1.0);
    bool success = false;
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        success = routeMap_[routeID].routeSetProgress(validProgress);
    }
    dirty_ = true;
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

void RouteManager::appendStats(const std::string& str) {
    ss_<<str;
}

const std::string RouteManager::getStats() {
    return ss_.str();
}

void RouteManager::clearStats() {
    ss_.clear();
}


void RouteManager::finalize() {
    using namespace mbgl::style;
    using namespace mbgl::style::expression;
    stats_.numFinalizedInvoked++;
    assert(style_ != nullptr);
    if(style_ != nullptr && dirty_) {
        //create the layers and geojsonsource for base route
        for(const auto& iter : routeMap_) {
            const auto& route = iter.second;
            const auto& routeID = iter.first;

            std::string baseLayerName = getBaseRouteLayerName(routeID);
            std::string baseGeoJSONSourceName = getBaseGeoJSONsourceName(routeID);

            if(style_->getSource(baseGeoJSONSourceName) == nullptr) {

                mapbox::geojsonvt::feature_collection featureCollection;
                const auto& geom = route.getGeometry();
                featureCollection.emplace_back(geom);

                GeoJSONOptions opts;
                opts.lineMetrics = true;
                std::unique_ptr<GeoJSONSource> geoJSONsrc = std::make_unique<GeoJSONSource>(baseGeoJSONSourceName, mbgl::makeMutable<mbgl::style::GeoJSONOptions>(std::move(opts))) ;
                geoJSONsrc->setGeoJSON(featureCollection);

                style_->addSource(std::move(geoJSONsrc));
            }

            //create the layers for each route
            if(style_->getLayer(baseLayerName) == nullptr) {
                std::unique_ptr<style::LineLayer> baselayer = std::make_unique<style::LineLayer>(baseLayerName, baseGeoJSONSourceName);
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
        }

        //create the layers, geojsonsource and gradients for active route
        for(auto& iter : routeMap_) {
            auto& route = iter.second;
            const auto& routeID = iter.first;
            std::string activeLayerName = getActiveRouteLayerName(routeID);
            std::string activeGeoJSONSourceName = getActiveGeoJSONsourceName(routeID);

            //create the geojson source for each route
            if(style_->getSource(activeGeoJSONSourceName) == nullptr) {
                mapbox::geojsonvt::feature_collection featureCollection;
                const auto& geom = iter.second.getGeometry();
                featureCollection.emplace_back(geom);

                GeoJSONOptions opts;
                opts.lineMetrics = true;
                std::unique_ptr<GeoJSONSource> geoJSONsrc = std::make_unique<GeoJSONSource>(activeGeoJSONSourceName, mbgl::makeMutable<mbgl::style::GeoJSONOptions>(std::move(opts))) ;
                geoJSONsrc->setGeoJSON(featureCollection);

                style_->addSource(std::move(geoJSONsrc));
            }

            //create the layers for each route
            if(style_->getLayer(activeLayerName) == nullptr) {
                std::unique_ptr<style::LineLayer> activelayer = std::make_unique<style::LineLayer>(activeLayerName, activeGeoJSONSourceName);
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


            const auto& createGradientExpression = [](const std::map<double, mbgl::Color>& gradient) {
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

                return expression;
            };

            //Create the gradient colors expressions and set on the active layer
            if(route.getGradientDirty()) {
                std::string baseLayerName = BASE_ROUTE_LAYER + std::to_string(iter.first.id);

                //create the gradient expression for active route.
                std::map<double, mbgl::Color> activeLayerGradient = route.getRouteSegmentColorStops(routeOptions_.innerColor);
                std::unique_ptr<expression::Expression> activeLayerExpression = createGradientExpression(activeLayerGradient);

                ColorRampPropertyValue activeColorRampProp(std::move(activeLayerExpression));
                Layer* activeRouteLayer = style_->getLayer(activeLayerName);
                LineLayer* activeRouteLineLayer = static_cast<LineLayer*>(activeRouteLayer);
                activeRouteLineLayer->setLineGradient(activeColorRampProp);

                //create the gradient expression for the base route
                std::map<double, mbgl::Color> baseLayerGradient = route.getRouteColorStops(routeOptions_.outerColor);
                std::unique_ptr<expression::Expression> baseLayerExpression = createGradientExpression(baseLayerGradient);

                ColorRampPropertyValue baseColorRampProp(std::move(baseLayerExpression));
                Layer* baseRouteLayer = style_->getLayer(baseLayerName);
                LineLayer* baseRouteLineLayer = static_cast<LineLayer*>(baseRouteLayer);
                baseRouteLineLayer->setLineGradient(baseColorRampProp);

                route.validateGradientDirty();
            }
        }

        //create the layer for the puck
        if(puckDirty_) {
            if(style_->getLayer(PUCK_LAYER_ID) == nullptr && activePuckID_.isValid()) {

                const auto& premultiply = [](mbgl::Color c)-> mbgl::Color {
                    c.r *= c.a;
                    c.g *= c.a;
                    c.b *= c.a;
                    return c;
                };


                auto puckLayer = std::make_unique<mbgl::style::LocationIndicatorLayer>(PUCK_LAYER_ID);
                puckLayer->setLocationTransition(mbgl::style::TransitionOptions(mbgl::Duration::zero(),mbgl::Duration::zero()));

                puckLayer->setAccuracyRadius(50);
                puckLayer->setAccuracyRadiusColor(
                    premultiply(mbgl::Color{0.0f, 1.0f, 0.0f, 0.2f})); // Note: these must be fed premultiplied

                puckLayer->setBearingTransition(mbgl::style::TransitionOptions(mbgl::Duration::zero(), mbgl::Duration::zero()));
                puckLayer->setBearing(mbgl::style::Rotation(0.0));
                puckLayer->setAccuracyRadiusBorderColor(premultiply(mbgl::Color{0.0f, 1.0f, 0.2f, 0.4f}));
                puckLayer->setTopImageSize(0.18f);
                puckLayer->setBearingImageSize(0.26f);
                puckLayer->setShadowImageSize(0.2f);
                puckLayer->setImageTiltDisplacement(7.0f); // set to 0 for a "flat" puck
                puckLayer->setPerspectiveCompensation(0.9f);

                //add the puck images to the style
                const PuckOptions& popts = puckMap_[activePuckID_].getPuckOptions();
                std::string bearingLocation = popts.locations.at(PuckImageType::pitBearing).fileLocation + "/"+popts.locations.at(PuckImageType::pitBearing).fileName;
                std::string bearingName = popts.locations.at(PuckImageType::pitBearing).fileName;
                std::unique_ptr<mbgl::style::Image> bearing = std::make_unique<mbgl::style::Image>(bearingName, mbgl::decodeImage(mbgl::util::read_file(bearingLocation)), 1.0f);
                style_->addImage(std::move(bearing));

                std::string shadowLocation = popts.locations.at(PuckImageType::pitShadow).fileLocation + "/"+popts.locations.at(PuckImageType::pitShadow).fileName;
                std::string shadowName = popts.locations.at(PuckImageType::pitShadow).fileName;
                std::unique_ptr<mbgl::style::Image> shadow = std::make_unique<mbgl::style::Image>(shadowName, mbgl::decodeImage(mbgl::util::read_file(shadowLocation)), 1.0f);
                style_->addImage(std::move(shadow));

                std::string topLocation = popts.locations.at(PuckImageType::pitTop).fileLocation + "/"+popts.locations.at(PuckImageType::pitTop).fileName;
                std::string topName = popts.locations.at(PuckImageType::pitTop).fileName;
                std::unique_ptr<mbgl::style::Image> top = std::make_unique<mbgl::style::Image>(topName, mbgl::decodeImage(mbgl::util::read_file(topLocation)), 1.0f);
                style_->addImage(std::move(top));

                //add puck image expression to the layer
                puckLayer->setBearingImage(mbgl::style::expression::Image(bearingName));
                puckLayer->setShadowImage(mbgl::style::expression::Image(shadowName));
                puckLayer->setTopImage(mbgl::style::expression::Image(topName));

                //add the puck layer to the style
                style_->addLayer(std::move(puckLayer));
            }

            //set the location of the puck
            if(activePuckID_.isValid()) {

                const auto& toArray = [](const mbgl::LatLng &crd) ->std::array<double, 3> {
                    return {crd.latitude(), crd.longitude(), 0};
                };

                mbgl::style::LocationIndicatorLayer* locationLayer = static_cast<LocationIndicatorLayer*>(style_->getLayer(PUCK_LAYER_ID));
                mbgl::LatLng mapCenter = {0.0, 0.0};
                locationLayer->setLocation(toArray(mapCenter));
            }

            puckDirty_ = false;
        }

        dirty_ = false;
    }
}

void RouteManager::puckSetActive(const PuckID& puckID) {
    activePuckID_ = puckID;
}

PuckID RouteManager::puckCreate(const PuckOptions& popts) {
    PuckID pid;
    bool success = puckIDpool_.CreateID((pid.id));
    if (success && pid.isValid()) {
        Puck puck(popts);
        puckMap_[pid] = puck;
        stats_.numPucks++;
    }

    return pid;
}

bool RouteManager::puckSetRoute(const RouteID& routeID) {
    assert(routeID.isValid() && "invalid route ID");
    if(routeID.isValid()) {
        routePuck_ = std::make_pair(routeID, activePuckID_);
        puckDirty_ = true;
        return true;
    }

    return false;
}

bool RouteManager::puckDispose(const PuckID& puckID) {
    assert(puckID.isValid() && "invalid puck ID");
    if(puckID.isValid() && puckMap_.find(puckID) != puckMap_.end()) {
        puckMap_.erase(puckID);

        //TODO: remove layer from style,
        //TODO: remove puck images from style

        return true;
    }

    return false;
}

bool RouteManager::puckSetVisible(const PuckID& puckID, bool onOff) {
    assert(puckID.isValid() && "invalid puck ID");
    if(puckID.isValid() && puckMap_.find(puckID) != puckMap_.end()) {
        puckMap_.at(puckID).setVisible(onOff);
        //TODO: handle in finalize
        puckDirty_ = true;
    }

    return false;
}

RouteID RouteManager::puckGetRoute() const {
    RouteID routeID;
    if(activePuckID_.isValid() && routePuck_.second == activePuckID_) {
        routeID = routePuck_.first;
    }

    return routeID;
}

RouteManager::~RouteManager() {}

}
}