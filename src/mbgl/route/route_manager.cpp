
#include "mbgl/util/containers.hpp"
#include "mbgl/util/math.hpp"

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
#include <iomanip> // For formatting
#include <iterator>

namespace mbgl {
namespace route {

const std::string RouteManager::BASE_ROUTE_LAYER = "base_route_layer_";
const std::string RouteManager::ACTIVE_ROUTE_LAYER = "active_route_layer_";
const std::string RouteManager::GEOJSON_BASE_ROUTE_SOURCE_ID = "base_route_geojson_source_";
const std::string RouteManager::GEOJSON_ACTIVE_ROUTE_SOURCE_ID = "active_route_geojson_source_";

namespace {

std::string gentabs(uint32_t tabcount) {
    std::string tabstr;
    for(size_t i = 0; i < tabcount; ++i) {
        tabstr += "\t";
    }
    return tabstr;
}

std::string embeddAPIcaptures(const std::vector<std::string>& apiCaptures) {
    std::stringstream ss;
    ss<<"{"<<std::endl;
    ss<<gentabs(1)<<"\"apiCalls\":["<<std::endl;
    for(size_t i = 0; i < apiCaptures.size(); ++i) {
        ss<<gentabs(2)<<apiCaptures[i];
        if(i == (apiCaptures.size()-1)) {
            ss<<std::endl;
        } else {
            ss<<","<<std::endl;
        }
    }
    ss<<gentabs(1)<<"]"<<std::endl;
    ss<<"}"<<std::endl;

    return ss.str();
}

std::string createAPIcapture(const std::string& fnname, const std::unordered_map<std::string, std::string>& args,
    const std::string& resultType, const std::string& resultValue, const std::unordered_map<std::string, std::string>& extraDataMap,
    const std::string& extraData = "") {
    std::stringstream tss;
    // Format the time as a string
    static uint32_t eventID = 0;
    std::stringstream ss;

    ss<<"{"<<std::endl;
    ss<<gentabs(2)<<"\"event_id\" : "<<std::to_string(eventID++)<<","<<std::endl;
    ss<<gentabs(2)<<"\"api_name\" : \""<<fnname<<"\","<<std::endl;
    ss<<gentabs(2)<<"\"parameters\" : {"<<std::endl;
    for(auto iter = args.begin(); iter != args.end(); ++iter) {
        std::string terminatingStr = std::next(iter) == args.end() ? "" : ",";
        ss<<gentabs(3)<<"\""<<iter->first<<"\" : "<<iter->second<<terminatingStr<<std::endl;
    }
    ss<<gentabs(2)<<"},"<<std::endl;
    ss<<gentabs(2)<<"\"extra_data\" : "<<"\""<<extraData<<"\","<<std::endl;
    ss<<gentabs(2)<<"\"extra_data_map\" : "<<"{"<<std::endl;
    for(auto iter = extraDataMap.begin(); iter != extraDataMap.end(); ++iter) {
        std::string terminatingStr = std::next(iter) == extraDataMap.end() ? "" : ",";
        ss<<gentabs(3)<<"\""<<iter->first<<"\" : "<<iter->second<<terminatingStr<<std::endl;
    }
    ss<<gentabs(2)<<"},"<<std::endl;
    ss<<gentabs(2)<<"\"result\" : {"<<std::endl;
    ss<<gentabs(2)<<"\"result_type\" : "<<"\""<<resultType<<"\","<<std::endl;
    ss<<gentabs(2)<<"\"result_value\" : "<<"\""<<resultValue<<"\""<<std::endl;
    ss<<gentabs(2)<<"}"<<std::endl;
    ss<<gentabs(1)<<"}";
    return ss.str();
}

#define TRACE_ROUTE_CALL(apiCaptures, functionParamMap, resultType, resultValue, extraDataMap, extraData) \
apiCaptures.push_back(createAPIcapture(__FUNCTION__, functionParamMap, resultType, resultValue, extraDataMap, extraData));

std::string formatElapsedTime(long long value) {
    std::stringstream ss;
    ss.imbue(std::locale("")); // Use the user's default locale for number formatting
    ss << std::fixed << value;
    return ss.str();
}

std::string toString(bool onOff) {
    return onOff ? "true" : "false";
}

std::string toString(const LineString<double>& line, uint32_t tabcount) {
    std::stringstream ss;
    ss<<gentabs(tabcount)<<"["<<std::endl;
    for(size_t i = 0; i < line.size(); i++) {
        std::string terminatingCommaStr = i == line.size() - 1 ? "" : ", ";
        ss<<gentabs(tabcount+1)<<"["<<std::to_string(line[i].x)<<", "<<std::to_string(line[i].y)<<"]"<<terminatingCommaStr<<std::endl;
    }
    ss<<gentabs(tabcount)<<"]";

    return ss.str();
}

std::string toString(const RouteOptions& ropts, uint32_t tabcount) {
    std::stringstream ss;
    ss<<gentabs(tabcount)<<"{"<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"innerColor\": "<< "\"rgba("<<std::to_string(ropts.innerColor.r)<<", "<<std::to_string(ropts.innerColor.g)<<", "<<std::to_string(ropts.innerColor.b)<<", "<<std::to_string(ropts.innerColor.a)<<")\","<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"outerColor\": "<< "\"rgba("<<std::to_string(ropts.outerColor.r)<<", "<<std::to_string(ropts.outerColor.g)<<", "<<std::to_string(ropts.outerColor.b)<<", "<<std::to_string(ropts.outerColor.a)<<")\","<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"innerClipColor\": "<< "\"rgba("<<std::to_string(ropts.innerClipColor.r)<<", "<<std::to_string(ropts.innerClipColor.g)<<", "<<std::to_string(ropts.innerClipColor.b)<<", "<<std::to_string(ropts.innerClipColor.a)<<")\","<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"outerClipColor\": "<< "\"rgba("<<std::to_string(ropts.outerClipColor.r)<<", "<<std::to_string(ropts.outerClipColor.g)<<", "<<std::to_string(ropts.outerClipColor.b)<<", "<<std::to_string(ropts.outerClipColor.a)<<")\","<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"innerWidth\": "<< "\""<<std::to_string(ropts.innerWidth)<<"\","<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"outerWidth\": "<< "\""<<std::to_string(ropts.outerWidth)<<"\","<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"layerBefore\": "<< "\""<<(ropts.layerBefore)<<"\""<<std::endl;
    ss <<gentabs(tabcount)<<"}";
    return ss.str();
}

std::string toString(const RouteSegmentOptions& rsopts, uint32_t tabcount) {
    std::stringstream ss;
    ss<<gentabs(tabcount)<<"{"<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"color\" : "<< "["<<std::to_string(rsopts.color.r)<<", "<<std::to_string(rsopts.color.g)<<", "<<std::to_string(rsopts.color.b)<<", "<<std::to_string(rsopts.color.a)<<"],"<<std::endl;
    ss<<gentabs(tabcount+1)<<"\"geometry\" : "<<toString(rsopts.geometry, tabcount+1)<<std::endl;
    ss <<gentabs(tabcount)<<"}";
    return ss.str();
}

std::string toString(const std::map<double, mbgl::Color>& gradient) {
    std::stringstream ss;
    ss<<"["<<std::endl;
    for(auto iter = gradient.begin(); iter != gradient.end(); iter++) {
        std::string terminatingStr = std::next(iter) == gradient.end() ? "" : ", ";
        ss<<"{\""<<std::to_string(iter->first)<<"\" : "<<"["
        <<std::to_string(iter->second.r)<<", "
        <<std::to_string(iter->second.g)<<", "
        <<std::to_string(iter->second.b)<<", "
        <<std::to_string(iter->second.a)<<"] }"
        <<terminatingStr
        <<std::endl;
    }
    ss<<"]";

    return ss.str();
}

LineString<double> removeDups(const LineString<double>& line, uint32_t* numDups) {
    if(line.empty()) return {};
    const double EPSILON = 1e-6;
    LineString<double> result;
    result.push_back(line[0]);
    uint32_t dupsCounts = 0;
    for(size_t i = 1; i < line.size(); i++) {
        mbgl::Point<double> currpt = line[i];
        mbgl::Point<double> prevpt = line[i-1];
        double distance = mbgl::util::dist<double>(currpt, prevpt);
        if(distance < EPSILON) {
            dupsCounts++;
        }
        else {
            result.push_back(line[i]);
        }
    }

    if(numDups != nullptr) {
        *numDups = dupsCounts;
    }

    return result;
}

}

RouteManager::RouteManager()
    : routeIDpool_(100) {
    }

std::string RouteManager::dirtyTypeToString(const RouteManager::DirtyType& dt) const {
    switch(dt) {
        case DirtyType::dtRouteGeometry:
            return "\"dtRouteGeometry\"";
        case DirtyType::dtRouteProgress:
            return "\"dtRouteProgress\"";
        case DirtyType::dtRouteSegments:
            return "\"dtRouteSegments\"";
    }

    return "";
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
            if(capturing_) {
                std::string url = style.getURL();
                const std::unordered_map<std::string, std::string> params = {{"style", url}};
                TRACE_ROUTE_CALL(apiCalls_, params, "void", "NA", {}, "NA");
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
        uint32_t numDups = 0;
        const LineString<double> geom = removeDups(geometry, &numDups);

        if(capturing_) {
            const std::unordered_map<std::string, std::string> params = {
                {"geometry", toString(geometry, 0)},
                {"routeOptions", toString(ropts, 0)}
            };
            std::string extraData = "numDups: "+std::to_string(numDups)+", numPoints: "+std::to_string(geom.size());
            TRACE_ROUTE_CALL(apiCalls_, params, "RouteID", std::to_string(rid.id), {}, extraData)
        }
        Route route(geom, ropts);
        routeMap_[rid] = route;
        stats_.numRoutes++;
        dirtyRouteMap_[DirtyType::dtRouteGeometry].insert(rid);
    }

    return rid;
}

bool RouteManager::routeSegmentCreate(const RouteID& routeID, const RouteSegmentOptions& routeSegOpts) {
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {

        RouteSegmentOptions rsegopts;
        rsegopts.color = routeSegOpts.color;
        rsegopts.sortOrder = routeSegOpts.sortOrder;
        uint32_t dupsCount = 0;
        rsegopts.geometry = removeDups(routeSegOpts.geometry, &dupsCount);

        if(capturing_) {
            std::string successStr;
            if(rsegopts.geometry.size() < 2) {
                successStr += "Failure due to less points";
            } else {
                successStr += "Success";
            }

            std::string extraData = "numDuplicates: "+std::to_string(dupsCount)+
                                    ", currTotalSegments: "+std::to_string(routeMap_[routeID].getNumRouteSegments());

            const std::unordered_map<std::string, std::string> params = {
                {"routeID", std::to_string(routeID.id)},
                {"routeSegOpts", toString(routeSegOpts, 0)}
            };
            TRACE_ROUTE_CALL(apiCalls_, params, "bool", successStr, {}, extraData)
        }

        //route segments must have atleast 2 points
        if(rsegopts.geometry.size() < 2) {
            return false;
        }

        routeMap_[routeID].routeSegmentCreate(rsegopts);
        stats_.numRouteSegments++;

        validateAddToDirtyBin(routeID, DirtyType::dtRouteSegments);

        return true;
    }

    return false;
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
        if(capturing_) {
            const std::unordered_map<std::string, std::string> params = {
                {"routeID", std::to_string(routeID.id)}
            };
            TRACE_ROUTE_CALL(apiCalls_, params, "void", "NA", {}, "NA")
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
    if(capturing_) {
        std::string validStyleStr = style_ != nullptr ? "true" : "false";
        std::string routeResidentStr = routeMap_.find(routeID) != routeMap_.end() ? "true" : "false";
        const std::unordered_map<std::string, std::string> params = {{"routeID", std::to_string(routeID.id)}};
        std::string extraData = "validStyle: "+validStyleStr+", routeResident: "+routeResidentStr;
        TRACE_ROUTE_CALL(apiCalls_, params, "bool", "NA", {}, extraData);
    }
    bool success = false;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end() && style_ != nullptr) {
        std::string baseLayerName = getBaseRouteLayerName(routeID);
        std::string activeLayerName = getActiveRouteLayerName(routeID);
        std::string baseGeoJSONsrcName = getBaseGeoJSONsourceName(routeID);
        std::string activeGeoJSONsrcName = getActiveGeoJSONsourceName(routeID);

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
        routeIDpool_.destroyID(routeID.id);
        stats_.numRoutes--;
        if(capturing_) {
            const std::unordered_map<std::string, std::string> params = {{"routeID", std::to_string(routeID.id)}};
            TRACE_ROUTE_CALL(apiCalls_, params, "bool", toString(success), {}, "NA");
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
        if(capturing_) {
            const std::unordered_map<std::string, std::string> params = {
                {"routeID", std::to_string(routeID.id)},
                {"progress", std::to_string(progress)}
            };
            TRACE_ROUTE_CALL(apiCalls_, params, "bool", "true", {}, "NA");
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
        double progressPercent = routeMap_.at(routeID).getProgressPercent(progressPoint);
        routeMap_[routeID].routeSetProgress(progressPercent);

        if(capturing_) {
            const std::unordered_map<std::string, std::string> params = {
                {"routeID", std::to_string(routeID.id)},
                {"progressPoint", std::to_string(progressPoint.x)+", "+std::to_string(progressPoint.y)}
            };
            const std::string extraData = "\nprogressPecent: "+std::to_string(progressPercent);
            TRACE_ROUTE_CALL(apiCalls_, params, "bool", "true", {}, extraData);
        }

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
}

const std::string RouteManager::endCapture() {
//    capturing_ = false;
    return embeddAPIcaptures(apiCalls_);
}

void RouteManager::finalizeRoute(const RouteID& routeID, const DirtyType& dt) {

    assert(routeID.isValid() && "invalid route ID");
    using namespace mbgl::style;
    using namespace mbgl::style::expression;
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
        } else if(layerID != routeOpts.layerBefore) {
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

    std::string captureExtraDataStr;
    if(routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {

        bool updateRouteLayers = false;
        bool updateGradients = false;
        bool updateProgress = false;
        switch (dt) {
            case DirtyType::dtRouteGeometry: {
                updateRouteLayers = true;
                updateGradients = true;
                updateProgress = true;
                if(capturing_) {
                    captureExtraDataStr += "dirty route geometry";
                }
            }
            break;

            case DirtyType::dtRouteProgress: {
                updateProgress = true;
                if(capturing_) {
                    captureExtraDataStr += "dirty route progress";
                }
            }
            break;

            case DirtyType::dtRouteSegments: {
                updateGradients = true;
                if(capturing_) {
                    captureExtraDataStr += "dirty route segments";
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
        std::unordered_map<std::string, std::string> gradientDebugMap;
        if (updateGradients) {
            // create the gradient expression for active route.
            std::map<double, mbgl::Color> gradientMap = route.getRouteSegmentColorStops(routeOptions.innerColor);

            if(capturing_) {
                gradientDebugMap["active_gradient_map"] = toString(gradientMap);
            }
            std::unique_ptr<expression::Expression> gradientExpression = createGradientExpression(
                gradientMap);

            ColorRampPropertyValue activeColorRampProp(std::move(gradientExpression));
            activeRouteLineLayer->setLineGradient(activeColorRampProp);

            // create the gradient expression for the base route
            std::map<double, mbgl::Color> baseLayerGradient = route.getRouteColorStops(routeOptions.outerColor);
            if(capturing_) {
                gradientDebugMap["base_gradient_map"] = toString(baseLayerGradient);
            }
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

        if(capturing_) {
            const std::unordered_map<std::string, std::string> params = {
                {"routeID", std::to_string(routeID.id)},
                {"dt", dirtyTypeToString(dt)}
            };
            TRACE_ROUTE_CALL(apiCalls_, params, "void", "NA", gradientDebugMap, captureExtraDataStr);
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