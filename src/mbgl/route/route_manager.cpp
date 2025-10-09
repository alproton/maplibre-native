
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
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <chrono>
#include <iomanip> // For formatting
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

namespace mbgl {
namespace route {
const std::string RouteManager::CASING_ROUTE_LAYER = "base_route_layer_";
const std::string RouteManager::ACTIVE_ROUTE_LAYER = "active_route_layer_";
const std::string RouteManager::GEOJSON_CASING_ROUTE_SOURCE_ID = "base_route_geojson_source_";
const std::string RouteManager::GEOJSON_ACTIVE_ROUTE_SOURCE_ID = "active_route_geojson_source_";

namespace {
std::string tabs(uint32_t tabcount) {
    std::string tabstr;
    for (size_t i = 0; i < tabcount; ++i) {
        tabstr += "\t";
    }
    return tabstr;
}

[[maybe_unused]] std::string embeddAPIcaptures(std::deque<std::string>& apiCaptures) {
    std::stringstream ss;
    ss << "{" << std::endl;
    ss << tabs(1) << "\"apiCalls\":[" << std::endl;
    while (!apiCaptures.empty()) {
        std::string apiCapture = apiCaptures.front();
        apiCaptures.pop_front();
        ss << tabs(2) << apiCapture;
        if (apiCaptures.empty()) {
            ss << std::endl;
        } else {
            ss << "," << std::endl;
        }
    }
    ss << tabs(1) << "]" << std::endl;
    ss << "}" << std::endl;

    return ss.str();
}

[[maybe_unused]] std::string createAPIcapture(const std::string& fnname,
                                              const std::unordered_map<std::string, std::string>& args,
                                              const std::string& resultType,
                                              const std::string& resultValue,
                                              const std::unordered_map<std::string, std::string>& extraDataMap,
                                              const std::string& extraData = "") {
    std::stringstream tss;
    // Format the time as a string
    static uint32_t eventID = 0;
    std::stringstream ss;

    ss << "{" << std::endl;
    ss << tabs(2) << "\"event_id\" : " << std::to_string(eventID++) << "," << std::endl;
    ss << tabs(2) << "\"api_name\" : \"" << fnname << "\"," << std::endl;
    ss << tabs(2) << "\"parameters\" : {" << std::endl;
    for (auto iter = args.begin(); iter != args.end(); ++iter) {
        std::string terminatingStr = std::next(iter) == args.end() ? "" : ",";
        ss << tabs(3) << "\"" << iter->first << "\" : " << iter->second << terminatingStr << std::endl;
    }
    ss << tabs(2) << "}," << std::endl;
    ss << tabs(2) << "\"extra_data\" : " << "\"" << extraData << "\"," << std::endl;
    ss << tabs(2) << "\"extra_data_map\" : " << "{" << std::endl;
    for (auto iter = extraDataMap.begin(); iter != extraDataMap.end(); ++iter) {
        std::string terminatingStr = std::next(iter) == extraDataMap.end() ? "" : ",";
        ss << tabs(3) << "\"" << iter->first << "\" : " << iter->second << terminatingStr << std::endl;
    }
    ss << tabs(2) << "}," << std::endl;
    ss << tabs(2) << "\"result\" : {" << std::endl;
    ss << tabs(2) << "\"result_type\" : " << "\"" << resultType << "\"," << std::endl;
    ss << tabs(2) << "\"result_value\" : " << "\"" << resultValue << "\"" << std::endl;
    ss << tabs(2) << "}" << std::endl;
    ss << tabs(1) << "}";
    return ss.str();
}

inline void updateOrAddAPICapture(const std::string& functionName,
                                  std::deque<std::string>& apiCaptures,
                                  const std::unordered_map<std::string, std::string>& args,
                                  const std::string& resultType,
                                  const std::string& resultValue,
                                  const std::unordered_map<std::string, std::string>& extraDataMap,
                                  const std::string& extraData) {
    // List of high-frequency methods to deduplicate
    static const std::unordered_set<std::string> highFreqMethods = {
        "routeSetProgressPercent", "routeSetProgressPoint"
        // Add more methods here if needed
    };

    bool isHighFreq = highFreqMethods.find(functionName) != highFreqMethods.end();

    if (isHighFreq && !apiCaptures.empty()) {
        // Check if last entry is the same function
        const std::string& lastCall = apiCaptures.back();
        std::string searchPattern = "\"api_name\" : \"" + functionName + "\"";

        if (lastCall.find(searchPattern) != std::string::npos) {
            // Update the last entry
            std::string updatedCall = createAPIcapture(
                functionName, args, resultType, resultValue, extraDataMap, extraData);
            apiCaptures.back() = updatedCall;
            return; // Don't add new entry
        }
    }

    // Add new entry
    if (apiCaptures.size() >= 100) {
        apiCaptures.pop_front();
    }

    apiCaptures.push_back(createAPIcapture(functionName, args, resultType, resultValue, extraDataMap, extraData));
}

#define TRACE_ROUTE_CALL(apiCaptures, functionParamMap, resultType, resultValue, extraDataMap, extraData) \
    updateOrAddAPICapture(__FUNCTION__, apiCaptures, functionParamMap, resultType, resultValue, extraDataMap, extraData)

[[maybe_unused]] std::string toString(bool onOff) {
    return onOff ? "true" : "false";
}

[[maybe_unused]] std::string toString(const LineString<double>& line, uint32_t tabcount = 0, int limit = -1) {
    std::stringstream ss;
    ss << tabs(tabcount) << "[" << std::endl;
    size_t lineLimit = limit < 0 ? line.size() : limit;
    lineLimit = std::min(lineLimit, line.size());
    for (size_t i = 0; i < lineLimit; i++) {
        std::string terminatingCommaStr = i == line.size() - 1 ? "" : ",";
        ss << tabs(tabcount + 1) << "[" << std::to_string(line[i].x) << ", " << std::to_string(line[i].y) << "]"
           << terminatingCommaStr << std::endl;
    }
    ss << tabs(tabcount) << "]";

    return ss.str();
}

[[maybe_unused]] std::string toString(const RouteSegmentOptions& routeSegOpts, uint32_t tabcount = 0, int limit = -1) {
    std::stringstream ss;
    ss << tabs(tabcount) << "{" << std::endl;
    ss << tabs(tabcount + 1) << "\"firstIndex\": " << "\"" << std::to_string(routeSegOpts.firstIndex) << "\","
       << std::endl;
    ss << tabs(tabcount + 1) << "\"firstIndexFraction\": " << "\"" << std::to_string(routeSegOpts.firstIndexFraction)
       << "\"," << std::endl;
    ss << tabs(tabcount + 1) << "\"lastIndex\": " << "\"" << std::to_string(routeSegOpts.lastIndex) << "\","
       << std::endl;
    ss << tabs(tabcount + 1) << "\"lastIndexFraction\": " << "\"" << std::to_string(routeSegOpts.lastIndexFraction)
       << "\"," << std::endl;
    ss << tabs(tabcount + 1) << "\"priority\": " << "\"" << toString(routeSegOpts.priority) << "\"" << std::endl;
    ss << tabs(tabcount + 1) << "\"color\": " << "[" << std::to_string(routeSegOpts.color.r) << ", "
       << std::to_string(routeSegOpts.color.g) << ", " << std::to_string(routeSegOpts.color.b) << ", "
       << std::to_string(routeSegOpts.color.a) << "]," << std::endl;
    ss << tabs(tabcount + 1) << "\"outerColor\": " << "[" << std::to_string(routeSegOpts.outerColor.r) << ", "
       << std::to_string(routeSegOpts.outerColor.g) << ", " << std::to_string(routeSegOpts.outerColor.b) << ", "
       << std::to_string(routeSegOpts.outerColor.a) << "]," << std::endl;
    size_t geomLimit = limit < 0 ? routeSegOpts.geometry.size() : limit;
    ss << tabs(tabcount + 1) << "\"geometry\": " << toString(routeSegOpts.geometry, tabcount + 1, geomLimit)
       << std::endl;
    ss << tabs(tabcount) << "}";

    return ss.str();
}

[[maybe_unused]] std::string toString(const std::vector<double>& dlistl, uint32_t tabcount) {
    std::stringstream ss;
    ss << tabs(tabcount) << "[" << std::endl;
    for (size_t i = 0; i < dlistl.size(); i++) {
        std::string terminatingCommaStr = i == dlistl.size() - 1 ? "" : ",";
        ss << tabs(tabcount + 1) << std::to_string(dlistl[i]) << terminatingCommaStr << std::endl;
    }
    ss << tabs(tabcount) << "]";

    return ss.str();
}

[[maybe_unused]] std::string toString(const std::map<double, double>& mapdata) {
    std::stringstream ss;
    ss << "[" << std::endl;
    for (auto iter = mapdata.begin(); iter != mapdata.end(); iter++) {
        std::string terminatingStr = std::next(iter) == mapdata.end() ? "" : ", ";
        ss << "{\"" << std::to_string(iter->first) << "\" : " << iter->second << " }" << terminatingStr << std::endl;
    }
    ss << "]";

    return ss.str();
}

[[maybe_unused]] std::string toString(const RouteOptions& ropts, uint32_t tabcount) {
    std::stringstream ss;
    ss << tabs(tabcount) << "{" << std::endl;
    ss << tabs(tabcount + 1) << "\"innerColor\": " << "[" << std::to_string(ropts.innerColor.r) << ", "
       << std::to_string(ropts.innerColor.g) << ", " << std::to_string(ropts.innerColor.b) << ", "
       << std::to_string(ropts.innerColor.a) << "]," << std::endl;
    ss << tabs(tabcount + 1) << "\"outerColor\": " << "[" << std::to_string(ropts.outerColor.r) << ", "
       << std::to_string(ropts.outerColor.g) << ", " << std::to_string(ropts.outerColor.b) << ", "
       << std::to_string(ropts.outerColor.a) << "]," << std::endl;
    ss << tabs(tabcount + 1) << "\"innerClipColor\": " << "[" << std::to_string(ropts.innerClipColor.r) << ", "
       << std::to_string(ropts.innerClipColor.g) << ", " << std::to_string(ropts.innerClipColor.b) << ", "
       << std::to_string(ropts.innerClipColor.a) << "]," << std::endl;
    ss << tabs(tabcount + 1) << "\"outerClipColor\": " << "[" << std::to_string(ropts.outerClipColor.r) << ", "
       << std::to_string(ropts.outerClipColor.g) << ", " << std::to_string(ropts.outerClipColor.b) << ", "
       << std::to_string(ropts.outerClipColor.a) << "]," << std::endl;
    ss << tabs(tabcount + 1) << "\"innerWidth\": " << "\"" << std::to_string(ropts.innerWidth) << "\"," << std::endl;
    ss << tabs(tabcount + 1) << "\"outerWidth\": " << "\"" << std::to_string(ropts.outerWidth) << "\"," << std::endl;
    ss << tabs(tabcount + 1) << "\"layerBefore\": " << "\"" << (ropts.layerBefore) << "\"," << std::endl;
    if (!ropts.outerWidthZoomStops.empty()) {
        ss << tabs(tabcount + 1) << "\"outerWidthZoomStops\": " << toString(ropts.outerWidthZoomStops) << ", "
           << std::endl;
    }
    if (!ropts.innerWidthZoomStops.empty()) {
        ss << tabs(tabcount + 1) << "\"innerWidthZoomStops\": " << toString(ropts.innerWidthZoomStops) << ", "
           << std::endl;
    }
    ss << tabs(tabcount + 1) << "\"useDynamicWidths\": " << toString(ropts.useDynamicWidths) << std::endl;
    ss << tabs(tabcount) << "}";
    return ss.str();
}

[[maybe_unused]] std::string toString(const std::map<double, mbgl::Color>& gradient) {
    std::stringstream ss;
    ss << "[" << std::endl;
    for (auto iter = gradient.begin(); iter != gradient.end(); iter++) {
        std::string terminatingStr = std::next(iter) == gradient.end() ? "" : ", ";
        ss << "{\"" << std::to_string(iter->first) << "\" : " << "[" << std::to_string(iter->second.r) << ", "
           << std::to_string(iter->second.g) << ", " << std::to_string(iter->second.b) << ", "
           << std::to_string(iter->second.a) << "] }" << terminatingStr << std::endl;
    }
    ss << "]";

    return ss.str();
}

constexpr double PIXEL_DENSITY = 1.6;

constexpr double getZoomStepDownValue() {
    return PIXEL_DENSITY - 1.0;
}

constexpr double PIXEL_DENSITY_FACTOR = 1.0 / PIXEL_DENSITY;
constexpr double ROUTE_LINE_ZOOM_LEVEL_4 = 4 - getZoomStepDownValue();
constexpr double ROUTE_LINE_ZOOM_LEVEL_10 = 10 - getZoomStepDownValue();
constexpr double ROUTE_LINE_ZOOM_LEVEL_18 = 18 - getZoomStepDownValue();
constexpr double ROUTE_LINE_ZOOM_LEVEL_20 = 20 - getZoomStepDownValue();

constexpr double ROUTE_LINE_WEIGHT_6 = 6;
constexpr double ROUTE_LINE_WEIGHT_9 = 9;
constexpr double ROUTE_LINE_WEIGHT_16 = 16;
constexpr double ROUTE_LINE_WEIGHT_22 = 22;

constexpr double ROUTE_LINE_CASING_MULTIPLIER = 1.6 * PIXEL_DENSITY_FACTOR;
constexpr double ROUTE_LINE_MULTIPLIER = 1.0 * PIXEL_DENSITY_FACTOR;

std::map<double, double> getDefaultRouteLineCasingWeights() {
    return {{ROUTE_LINE_ZOOM_LEVEL_4, ROUTE_LINE_WEIGHT_6 * ROUTE_LINE_CASING_MULTIPLIER},
            {ROUTE_LINE_ZOOM_LEVEL_10, ROUTE_LINE_WEIGHT_9 * ROUTE_LINE_CASING_MULTIPLIER},
            {ROUTE_LINE_ZOOM_LEVEL_18, ROUTE_LINE_WEIGHT_16 * ROUTE_LINE_CASING_MULTIPLIER},
            {ROUTE_LINE_ZOOM_LEVEL_20, ROUTE_LINE_WEIGHT_22 * ROUTE_LINE_CASING_MULTIPLIER}};
}

std::map<double, double> getDefaultRouteLineWeights() {
    return {{ROUTE_LINE_ZOOM_LEVEL_4, ROUTE_LINE_WEIGHT_6 * ROUTE_LINE_MULTIPLIER},
            {ROUTE_LINE_ZOOM_LEVEL_10, ROUTE_LINE_WEIGHT_9 * ROUTE_LINE_MULTIPLIER},
            {ROUTE_LINE_ZOOM_LEVEL_18, ROUTE_LINE_WEIGHT_16 * ROUTE_LINE_MULTIPLIER},
            {ROUTE_LINE_ZOOM_LEVEL_20, ROUTE_LINE_WEIGHT_22 * ROUTE_LINE_MULTIPLIER}};
}

struct AvgIntervalStat {
    std::chrono::steady_clock::time_point lastStartTime;
    std::chrono::duration<double> totalIntervalDuration{0.0};
    long long intervalCount = 0;
    bool firstCall = true;
};

AvgIntervalStat routeCreateStat;
AvgIntervalStat routeSegmentCreateStat;

} // namespace

RouteManager::RouteManager()
    : routeIDpool_(100) {}

std::string RouteManager::dirtyTypeToString(const RouteManager::DirtyType& dt) const {
    switch (dt) {
        case DirtyType::dtRouteGeometry:
            return "\"dtRouteGeometry\"";
        case DirtyType::dtRouteProgress:
            return "\"dtRouteProgress\"";
        case DirtyType::dtRouteSegments:
            return "\"dtRouteSegments\"";
    }

    return "";
}

std::vector<RouteID> RouteManager::getAllRoutes() const {
    std::vector<RouteID> routeIDs;
    for (const auto& iter : routeMap_) {
        routeIDs.push_back(iter.first);
    }

    return routeIDs;
}

int RouteManager::getTopMost(const std::vector<RouteID>& routeList) const {
    assert(style_ != nullptr && "style not set");

    if (style_ != nullptr) {
        std::vector<style::Layer*> layers = style_->getLayers();

        if (!layers.empty()) {
            for (size_t i = layers.size() - 1; i >= 0; i--) {
                const std::string currLayerName = layers[i]->getID();

                for (size_t j = 0; j < routeList.size(); j++) {
                    const RouteID routeID = routeList[j];
                    const std::string& layerName = getBaseRouteLayerName(routeID);
                    if (layerName == currLayerName) {
                        return j;
                    }
                }
            }
        }
    }

    return -1;
}

std::string RouteManager::captureSnapshot() const {
    // TODO: use rapidjson to create the json string
    std::stringstream ss;
    ss << "{" << std::endl;
    ss << tabs(1) << "\"num_routes\" : " << std::to_string(routeMap_.size()) << "," << std::endl;
    ss << tabs(1) << "\"routes\": " << "[" << std::endl;
    for (auto iter = routeMap_.begin(); iter != routeMap_.end(); ++iter) {
        const RouteID& routeID = iter->first;
        const auto& capturedNavStops = iter->second.getCapturedNavStops();
        const auto& capturedNavPercents = iter->second.getCapturedNavPercent();
        std::string segmentCommaStr = !capturedNavStops.empty() || !capturedNavPercents.empty() ? "," : "";
        std::string geomCommaStr = segmentCommaStr;
        std::string terminatingStr = std::next(iter) == routeMap_.end() ? "" : ",";
        ss << tabs(2) << "{" << std::endl;
        ss << tabs(3) << "\"route_id\" : " << std::to_string(routeID.id) << "," << std::endl;
        ss << tabs(3) << "\"route\" : {" << std::endl;
        ss << tabs(4) << "\"route_options\" : " << std::endl;
        ss << toString(iter->second.getRouteOptions(), 5) << "," << std::endl;
        ss << tabs(4) << "\"geometry\" : " << std::endl;

        ss << toString(iter->second.getGeometry(), 4) << geomCommaStr << std::endl;
        if (iter->second.hasRouteSegments()) {
            ss << tabs(4) << "," << std::endl;
            ss << tabs(4) << "\"route_segments\" : " << std::endl;
            ss << iter->second.segmentsToString(5) << segmentCommaStr << std::endl;
        }
        if (!capturedNavStops.empty()) {
            ss << tabs(4) << "\"nav_stops\" : " << std::endl;
            ss << toString(capturedNavStops, 5);
        } else if (!capturedNavPercents.empty()) {
            ss << tabs(4) << "\"nav_stops_percent\" : " << std::endl;
            ss << toString(capturedNavPercents, 5);
        }
        ss << tabs(3) << "}" << std::endl;
        ss << tabs(2) << "}" << terminatingStr << std::endl;
    }
    ss << tabs(1) << "]" << std::endl;
    ss << "}" << std::endl;
    return ss.str();
}

void RouteManager::setStyle(style::Style& style) {
    if (style_ != nullptr && style_ != &style) {
        // remove the old base, active layer and source for each route and add them to the new stylef
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
        Route route(geometry, ropts);
        routeMap_[rid] = route;
        stats_.numRoutes++;
        dirtyRouteMap_[DirtyType::dtRouteGeometry].insert(rid);

        auto now = std::chrono::steady_clock::now();
        if (!routeCreateStat.firstCall) {
            auto interval = now - routeCreateStat.lastStartTime;
            routeCreateStat.totalIntervalDuration += interval;
            routeCreateStat.intervalCount++;
            if (routeCreateStat.intervalCount > 0) {
                stats_.avgRouteCreationInterval =
                    std::chrono::duration<double>(routeCreateStat.totalIntervalDuration).count() /
                    routeCreateStat.intervalCount;
            }

        } else {
            routeCreateStat.firstCall = false;
        }
        routeCreateStat.lastStartTime = now;
    }

    // trace
    std::string geometryStr = toString(geometry, 0, 5);
    std::string roptsStr = toString(ropts, 0);
    std::unordered_map<std::string, std::string> functionParamMap = {{"geometry", geometryStr},
                                                                     {"routeOptions", roptsStr}};
    TRACE_ROUTE_CALL(stats_.recentApiCalls, functionParamMap, "RouteID", std::to_string(rid.id), {}, "");

    return rid;
}

RouteID RouteManager::routePreCreate(const RouteID& routeID, uint32_t numRoutes) {
    assert(routeID.isValid() && "Invalid route ID");
    if (routeID.isValid()) {
        uint32_t id = routeID.id;
        bool success = routeIDpool_.createRangeID(id, numRoutes);
        if (success) {
            return routeID;
        }
    }

    return RouteID();
}

bool RouteManager::routeSet(const RouteID& routeID, const LineString<double>& geometry, const RouteOptions& ropts) {
    assert(routeID.isValid() && "Invalid route ID");
    assert(!geometry.empty() && "Invalid route geometry");
    if (routeID.isValid() && !geometry.empty() && routeMap_.find(routeID) != routeMap_.end()) {
        Route route(geometry, ropts);
        routeMap_[routeID] = route;
        stats_.numRoutes++;
        dirtyRouteMap_[DirtyType::dtRouteGeometry].insert(routeID);

        return true;
    }

    return false;
}

bool RouteManager::routeSegmentCreate(const RouteID& routeID, const RouteSegmentOptions& routeSegOpts) {
    assert(routeID.isValid() && "Invalid route ID");
    assert(routeMap_.find(routeID) != routeMap_.end() && "Route not found internally");
    bool success = false;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        // route segments must have atleast 2 points
        if (routeSegOpts.firstIndex == INVALID_UINT || routeSegOpts.lastIndex == INVALID_UINT ||
            routeSegOpts.firstIndexFraction < 0.0f || routeSegOpts.lastIndexFraction < 0.0f ||
            routeSegOpts.firstIndexFraction > 1.0f || routeSegOpts.lastIndexFraction > 1.0f) {
            return false;
        }

        success = routeMap_[routeID].routeSegmentCreate(routeSegOpts);
        if (success) {
            stats_.numRouteSegments++;
            validateAddToDirtyBin(routeID, DirtyType::dtRouteSegments);
        }
    }

    // trace
    std::string routeSegOptsStr = toString(routeSegOpts, 0, 5);
    std::unordered_map<std::string, std::string> functionParamMap = {{"RouteID", std::to_string(routeID.id)},
                                                                     {"routeSegmentOptions", routeSegOptsStr}};
    TRACE_ROUTE_CALL(stats_.recentApiCalls, functionParamMap, "bool", toString(success), {}, "");

    return success;
}

void RouteManager::captureNavStops(bool onOff) {
    captureNavStops_ = onOff;
}

bool RouteManager::isCaptureNavStopsEnabled() const {
    return captureNavStops_;
}

bool RouteManager::loadCapture(const std::string& capture) {
    rapidjson::Document document;
    rapidjson::ParseResult result = document.Parse(capture);

    if (document.HasParseError()) {
        const auto errorCode = document.GetParseError();
        std::stringstream ss;
        ss << "JSON parse error: " << rapidjson::GetParseError_En(result.Code()) << " (" << result.Offset() << ")";
        ss << "Error parsing JSON: " << errorCode << std::endl;
        Log::Error(Event::Route, ss.str());

        return false;
    }

    uint32_t numRoutes = 0;
    if (document.HasMember("num_routes") && document["num_routes"].IsInt()) {
        numRoutes = document["num_routes"].GetInt();
        Log::Info(Event::Route, "De-serializing " + std::to_string(numRoutes) + " routes");
    }

    // iterate through the document and get the minimum and maximum routeIDs, lets pre-create these IDs in the route
    // manager.
    uint32_t minRoute = INT_MAX;
    uint32_t maxRoute = 0;
    if (document.HasMember("routes") && document["routes"].IsArray()) {
        const rapidjson::Value& routes = document["routes"];
        for (rapidjson::SizeType i = 0; i < routes.Size(); i++) {
            const rapidjson::Value& route = routes[i];
            if (route.HasMember("route_id") && route["route_id"].IsInt()) {
                const rapidjson::Value& routeIDval = route["route_id"];
                uint32_t rid = routeIDval.GetInt();
                RouteID routeID = RouteID(rid);
                routeMap_[routeID] = {};

                if (minRoute > rid) minRoute = rid;
                if (maxRoute < rid) maxRoute = rid;
            }
        }
        RouteID minRouteID = RouteID(minRoute);
        uint32_t numRoutesIDs = maxRoute - minRoute + 1;
        routePreCreate(minRouteID, numRoutesIDs);
    }

    if (document.HasMember("routes") && document["routes"].IsArray()) {
        const rapidjson::Value& routes = document["routes"];
        for (rapidjson::SizeType i = 0; i < routes.Size(); i++) {
            const rapidjson::Value& route = routes[i];
            RouteID routeID;
            if (route.HasMember("route_id") && route["route_id"].IsInt()) {
                const rapidjson::Value& routeIDval = route["route_id"];
                uint32_t rid = routeIDval.GetInt();
                routeID = RouteID(rid);
            }
            assert(routeID.isValid() && "captured route ID not valid");

            if (route.HasMember("route") && route["route"].IsObject()) {
                const rapidjson::Value& route_obj = route["route"];

                mbgl::route::RouteOptions routeOpts;
                const auto& mbglColor = [](const rapidjson::Value& jsonColor) -> mbgl::Color {
                    std::array<float, 4> innerColorArr;
                    for (rapidjson::SizeType j = 0; j < jsonColor.Size(); j++) {
                        innerColorArr[j] = jsonColor[j].GetFloat();
                    }
                    mbgl::Color color(innerColorArr[0], innerColorArr[1], innerColorArr[2], innerColorArr[3]);

                    return color;
                };

                if (route_obj.HasMember("route_options") && route_obj["route_options"].IsObject()) {
                    const rapidjson::Value& route_options = route_obj["route_options"];

                    // innerColor
                    if (route_options.HasMember("innerColor") && route_options["innerColor"].IsArray()) {
                        const rapidjson::Value& innerColor = route_options["innerColor"];
                        routeOpts.innerColor = mbglColor(innerColor);
                    }

                    // outerColor
                    if (route_options.HasMember("outerColor") && route_options["outerColor"].IsArray()) {
                        const rapidjson::Value& outerColor = route_options["outerColor"];
                        routeOpts.outerColor = mbglColor(outerColor);
                    }

                    // innerClipColor
                    if (route_options.HasMember("innerClipColor") && route_options["innerClipColor"].IsArray()) {
                        const rapidjson::Value& innerClipColor = route_options["innerClipColor"];
                        routeOpts.innerClipColor = mbglColor(innerClipColor);
                    }

                    // outerClipColor
                    if (route_options.HasMember("outerClipColor") && route_options["outerClipColor"].IsArray()) {
                        const rapidjson::Value& outerClipColor = route_options["outerClipColor"];
                        routeOpts.outerClipColor = mbglColor(outerClipColor);
                    }

                    // innerWidth
                    if (route_options.HasMember("innerWidth") && route_options["innerWidth"].IsArray()) {
                        const rapidjson::Value& innerWidth = route_options["innerWidth"];
                        routeOpts.innerWidth = innerWidth.GetFloat();
                    }

                    // outerWidth
                    if (route_options.HasMember("outerWidth") && route_options["outerWidth"].IsArray()) {
                        const rapidjson::Value& outerWidth = route_options["outerWidth"];
                        routeOpts.outerWidth = outerWidth.GetFloat();
                    }

                    // layerBefore
                    if (route_options.HasMember("layerBefore") && route_options["layerBefore"].IsArray()) {
                        const rapidjson::Value& layerBefore = route_options["layerBefore"];
                        routeOpts.layerBefore = layerBefore.GetString();
                    }

                    // useDynamicWidths
                    if (route_options.HasMember("useDynamicWidths") && route_options["useDynamicWidths"].IsArray()) {
                        const rapidjson::Value& useDynamicWidths = route_options["useDynamicWidths"];
                        routeOpts.useDynamicWidths = useDynamicWidths.GetBool();
                    }

                    // TODO: add this to route capture
                    routeOpts.useMercatorProjection = true;
                }

                mbgl::LineString<double> route_geom;
                if (route_obj.HasMember("geometry") && route_obj["geometry"].IsArray()) {
                    const rapidjson::Value& geometry = route_obj["geometry"];
                    for (rapidjson::SizeType j = 0; j < geometry.Size(); j++) {
                        const rapidjson::Value& point = geometry[j];
                        if (point.IsArray() && point.Size() == 2) {
                            double x = point[0].GetDouble();
                            double y = point[1].GetDouble();
                            route_geom.push_back({x, y});
                        }
                    }
                }

                bool success = routeSet(routeID, route_geom, routeOpts);
                assert(success && "failed to create route on pre-created route ID");
                if (!success) {
                    Log::Warning(Event::Route, "failed to create route on pre-created route ID");
                }

                if (route_obj.HasMember("nav_stops") && route_obj["nav_stops"].IsArray()) {
                    const rapidjson::Value& nav_stops = route_obj["nav_stops"];
                    for (rapidjson::SizeType j = 0; j < nav_stops.Size(); j++) {
                        const rapidjson::Value& point = nav_stops[j];
                        if (point.IsArray() && point.Size() == 2) {
                            double x = point[0].GetDouble();
                            double y = point[1].GetDouble();
                            routeMap_[routeID].addNavStopPoint({x, y});
                        }
                    }
                    if (!vanishingRouteID_.isValid()) {
                        setVanishingRouteID(vanishingRouteID_);
                    }
                }

                if (route_obj.HasMember("nav_stops_percent") && route_obj["nav_stops_percent"].IsArray()) {
                    const rapidjson::Value& nav_stops_percent = route_obj["nav_stops_percent"];
                    for (rapidjson::SizeType j = 0; j < nav_stops_percent.Size(); j++) {
                        const rapidjson::Value& percent_value = nav_stops_percent[j];
                        if (percent_value.IsDouble()) {
                            double percent = percent_value.GetDouble();
                            routeMap_[routeID].addNavStopPercent(percent);
                        }
                    }
                }

                // if there is no vanishing route defined in the capture, it means we have not captured nav stops.
                // lets just set the first route in the map as the vanishing route.
                if (!vanishingRouteID_.isValid()) {
                    setVanishingRouteID(routeMap_.begin()->first);
                }

                // create route segments
                if (route_obj.HasMember("route_segments") && route_obj["route_segments"].IsArray()) {
                    const rapidjson::Value& route_segments = route_obj["route_segments"];
                    for (rapidjson::SizeType j = 0; j < route_segments.Size(); j++) {
                        mbgl::route::RouteSegmentOptions rsopts;
                        const rapidjson::Value& segment = route_segments[j];
                        if (segment.HasMember("route_segment_options") && segment["route_segment_options"].IsObject()) {
                            const rapidjson::Value& segment_options = segment["route_segment_options"];
                            if (segment_options.HasMember("color") && segment_options["color"].IsArray()) {
                                const rapidjson::Value& color = segment_options["color"];
                                rsopts.color = mbglColor(color);
                            }

                            if (segment_options.HasMember("outer_color") && segment_options["outer_color"].IsArray()) {
                                const rapidjson::Value& outerColor = segment_options["outer_color"];
                                rsopts.outerColor = mbglColor(outerColor);
                            }
                            // TODO: deprecated use of "geometry"
                            if (segment_options.HasMember("geometry") && segment_options["geometry"].IsArray()) {
                                const rapidjson::Value& geometry = segment_options["geometry"];
                                for (rapidjson::SizeType k = 0; k < geometry.Size(); k++) {
                                    const rapidjson::Value& point = geometry[k];
                                    if (point.IsArray() && point.Size() == 2) {
                                        double x = point[0].GetDouble();
                                        double y = point[1].GetDouble();
                                        rsopts.geometry.push_back({x, y});
                                    }
                                }
                            }

                            if (segment_options.HasMember("first_index")) {
                                const rapidjson::Value& firstIndexVal = segment_options["first_index"];
                                rsopts.firstIndex = firstIndexVal.GetUint();
                            }

                            if (segment_options.HasMember("first_index_fraction")) {
                                const rapidjson::Value& firstIndexFractionVal = segment_options["first_index_fraction"];
                                rsopts.firstIndexFraction = firstIndexFractionVal.GetFloat();
                            }

                            if (segment_options.HasMember("last_index")) {
                                const rapidjson::Value& lastIndexVal = segment_options["last_index"];
                                rsopts.lastIndex = lastIndexVal.GetUint();
                            }

                            if (segment_options.HasMember("last_index_fraction")) {
                                const rapidjson::Value& lastIndexFractionVal = segment_options["last_index_fraction"];
                                rsopts.lastIndexFraction = lastIndexFractionVal.GetFloat();
                            }

                            if (segment_options.HasMember("priority")) {
                                rsopts.priority = segment_options["priority"].GetInt();
                            }
                        }
                        routeSegmentCreate(routeID, rsopts);
                    }
                }
            }
        }
        finalize();
    }

    return true;
}

bool RouteManager::captureScrubRoute(double scrubValue,
                                     const ScrubOptions& scrubOpts,
                                     Point<double>* optPointOut,
                                     double* optBearingOut) {
    if (vanishingRouteID_.isValid() && routeMap_.find(vanishingRouteID_) != routeMap_.end()) {
        scrubValue = std::clamp(scrubValue, 0.0, 1.0);
        if (routeMap_[vanishingRouteID_].hasNavStopsPoints()) {
            const uint32_t currRouteCaptureProgressDiscrete = static_cast<uint32_t>(scrubValue);

            const mbgl::LineString<double>& navstops = routeMap_[vanishingRouteID_].getCapturedNavStops();
            const uint32_t sz = navstops.size();
            const auto& navStop = navstops[currRouteCaptureProgressDiscrete % sz];
            double percentage = routeSetProgressPoint(vanishingRouteID_, navStop, Precision::Fine);
            Log::Info(Event::Route, "scrubbed calculated percentage :" + std::to_string(percentage));
            if (optPointOut) {
                *optPointOut = navStop;
            }
            double bearing = 0.0;
            if (optBearingOut) {
                mbgl::Point<double> pt = getPoint(vanishingRouteID_, percentage, Precision::Fine, &bearing);
                *optPointOut = pt;
            }
            if (optBearingOut) {
                *optBearingOut = bearing;
            }
            finalize();

        } else if (routeMap_[vanishingRouteID_].hasNavStopsPercent()) {
            const uint32_t currRouteCaptureProgressDiscrete = static_cast<uint32_t>(scrubValue);

            const std::vector<double>& navstops = routeMap_[vanishingRouteID_].getCapturedNavPercent();
            const uint32_t sz = navstops.size();
            const auto& navStopPercent = navstops[currRouteCaptureProgressDiscrete % sz];
            routeSetProgressPercent(vanishingRouteID_, navStopPercent);
            Log::Info(Event::Route, "scrubbed calculated percentage :" + std::to_string(navStopPercent));
            double bearing = 0.0;
            if (optPointOut) {
                const auto& navstop = getPoint(vanishingRouteID_, navStopPercent, Precision::Fine, &bearing);
                *optPointOut = navstop;
            }
            if (optBearingOut) {
                *optBearingOut = bearing;
            }

            finalize();
        } else {
            // we may not have any nav stops captured, so lets just use the route geometry
            double bearing = 0.0;
            const auto& navstop = getPoint(vanishingRouteID_, scrubValue, Precision::Fine, &bearing);
            if (scrubOpts.fallbackPoint) {
                routeSetProgressPoint(vanishingRouteID_, navstop, Precision::Fine);
            } else {
                routeSetProgressPercent(vanishingRouteID_, scrubValue);
            }
            if (optPointOut) {
                *optPointOut = navstop;
            }
            if (optBearingOut) {
                *optBearingOut = bearing;
            }
            finalize();
        }
    }

    return true;
}

std::optional<LineString<double>> RouteManager::routeGetGeometry(const RouteID& routeID) const {
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        return routeMap_.at(routeID).getGeometry();
    }

    return std::nullopt;
}

void RouteManager::validateAddToDirtyBin(const RouteID& routeID, const DirtyType& dirtyBin) {
    // check if this route is in the list of dirty geometry routes. if so, then no need to set dirty segments, as it
    // will be created during finalize
    bool foundDirtyRoute = false;
    if (dirtyRouteMap_.find(DirtyType::dtRouteGeometry) != dirtyRouteMap_.end()) {
        const auto& dirtyRouteIDs = dirtyRouteMap_[DirtyType::dtRouteGeometry];
        if (dirtyRouteIDs.find(routeID) != dirtyRouteIDs.end()) {
            foundDirtyRoute = true;
        }
    }
    if (!foundDirtyRoute) {
        dirtyRouteMap_[dirtyBin].insert(routeID);
    }
}

void RouteManager::routeClearSegments(const RouteID& routeID) {
    assert(routeID.isValid() && "invalid route ID");
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        stats_.numRouteSegments -= routeMap_[routeID].getNumRouteSegments();
        if (routeMap_[routeID].hasRouteSegments()) {
            routeMap_[routeID].routeSegmentsClear();
            validateAddToDirtyBin(routeID, DirtyType::dtRouteSegments);
        }

        auto now = std::chrono::steady_clock::now();
        if (!routeSegmentCreateStat.firstCall) {
            auto interval = now - routeSegmentCreateStat.lastStartTime;
            routeSegmentCreateStat.totalIntervalDuration += interval;
            routeSegmentCreateStat.intervalCount++;
            if (routeSegmentCreateStat.intervalCount > 0) {
                stats_.avgRouteSegmentCreationInterval =
                    std::chrono::duration<double>(routeSegmentCreateStat.totalIntervalDuration).count() /
                    routeSegmentCreateStat.intervalCount;
            }

        } else {
            routeSegmentCreateStat.firstCall = false;
        }
        routeSegmentCreateStat.lastStartTime = now;
    }

    // trace
    std::unordered_map<std::string, std::string> functionParamMap = {{"RouteID", std::to_string(routeID.id)}};
    TRACE_ROUTE_CALL(stats_.recentApiCalls, functionParamMap, "void", "", {}, "");
}

bool RouteManager::routeDispose(const RouteID& routeID) {
    assert(style_ != nullptr && "Style not set!");
    assert(routeID.isValid() && "Invalid route ID");
    assert(routeMap_.find(routeID) != routeMap_.end() && "Route not found internally");
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

        // Clean up the progress tracking for this route
        previousProgressMap_.erase(routeID);
    }

    // trace
    std::unordered_map<std::string, std::string> functionParamMap = {{"RouteID", std::to_string(routeID.id)}};
    TRACE_ROUTE_CALL(stats_.recentApiCalls, functionParamMap, "bool", toString(success), {}, "");

    return success;
}

bool RouteManager::hasRoutes() const {
    return !routeMap_.empty();
}

bool RouteManager::routeSetProgressPercent(const RouteID& routeID, const double progress) {
    assert(style_ != nullptr && "Style not set!");
    assert(routeID.isValid() && "invalid route ID");
    double validProgress = std::clamp(progress, 0.0, 1.0);
    bool success = false;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        // Check for large delta in progress
        auto prevProgressIter = previousProgressMap_.find(routeID);
        if (prevProgressIter != previousProgressMap_.end()) {
            double previousProgress = prevProgressIter->second;
            double delta = std::abs(validProgress - previousProgress);

            // If delta exceeds threshold, increment counter
            if (delta > largeDeltaThreshold_) {
                stats_.numLargeDeltaVanishingPercents++;
            }
        }

        // Update the previous progress for this route
        previousProgressMap_[routeID] = validProgress;

        routeMap_[routeID].routeSetProgress(validProgress, captureNavStops_);
        validateAddToDirtyBin(routeID, DirtyType::dtRouteProgress);

        success = true;
    }

    // trace
    std::unordered_map<std::string, std::string> functionParamMap = {{"RouteID", std::to_string(routeID.id)},
                                                                     {"progress", std::to_string(progress)}};
    TRACE_ROUTE_CALL(stats_.recentApiCalls, functionParamMap, "bool", toString(success), {}, "");

    return success;
}

double RouteManager::routeSetProgressPoint(const RouteID& routeID,
                                           const mbgl::Point<double>& progressPoint,
                                           const Precision& precision) {
    assert(routeID.isValid() && "invalid route ID");
    double percentage = -1.0;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        auto startTime = std::chrono::high_resolution_clock::now();
        {
            if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
                percentage = routeMap_.at(routeID).getProgressPercent(progressPoint, precision, captureNavStops_);
            }
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = endTime - startTime;
        auto durationInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        totalVanishingRouteElapsedMillis += durationInMilliseconds.count();
        numVanisingRouteInvocations++;

        if (stats_.maxRouteVanishingElapsedMillis < durationInMilliseconds.count()) {
            stats_.maxRouteVanishingElapsedMillis = durationInMilliseconds.count();
        }
        if (stats_.minRouteVanishingElapsedMillis > durationInMilliseconds.count()) {
            stats_.minRouteVanishingElapsedMillis = durationInMilliseconds.count();
        }
        if (numVanisingRouteInvocations > 0) {
            stats_.avgRouteVanishingElapsedMillis = static_cast<double>(totalVanishingRouteElapsedMillis) /
                                                    static_cast<double>(numVanisingRouteInvocations);
        }

        if (percentage >= 0.0 && percentage <= 1.0) {
            routeMap_[routeID].routeSetProgress(percentage);
            validateAddToDirtyBin(routeID, DirtyType::dtRouteProgress);
        }
    }

    // trace
    std::string precisionStr = (precision == Precision::Coarse) ? "Coarse"
                               : (precision == Precision::Fine) ? "Fine"
                                                                : "Exact";
    std::unordered_map<std::string, std::string> functionParamMap = {
        {"RouteID", std::to_string(routeID.id)},
        {"progressPoint", std::to_string(progressPoint.x) + ", " + std::to_string(progressPoint.y)},
        {"precision", precisionStr}};
    TRACE_ROUTE_CALL(stats_.recentApiCalls, functionParamMap, "double", std::to_string(percentage), {}, "");

    return percentage;
}

mbgl::Point<double> RouteManager::getPoint(const RouteID& routeID,
                                           double percent,
                                           const Precision& precision,
                                           double* bearing) const {
    assert(routeID.isValid() && "invalid route ID");
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        return routeMap_.at(routeID).getPoint(percent, precision, bearing);
    }

    return {0.0, 0.0};
}

std::string RouteManager::getActiveRouteLayerName(const RouteID& routeID) const {
    return ACTIVE_ROUTE_LAYER + std::to_string(routeID.id);
}

std::string RouteManager::getBaseRouteLayerName(const RouteID& routeID) const {
    return CASING_ROUTE_LAYER + std::to_string(routeID.id);
}

std::string RouteManager::getActiveGeoJSONsourceName(const RouteID& routeID) const {
    return GEOJSON_ACTIVE_ROUTE_SOURCE_ID + std::to_string(routeID.id);
}

std::string RouteManager::getBaseGeoJSONsourceName(const RouteID& routeID) const {
    return GEOJSON_CASING_ROUTE_SOURCE_ID + std::to_string(routeID.id);
}

const std::string RouteManager::getStats() {
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    rapidjson::Value route_stats(rapidjson::kObjectType);
    route_stats.AddMember("num_routes", stats_.numRoutes, allocator);
    route_stats.AddMember("num_traffic_zones", stats_.numRouteSegments, allocator);
    route_stats.AddMember("num_finalize_invocations", stats_.numFinalizedInvoked, allocator);
    route_stats.AddMember("inconsistant_route_API_usages", stats_.inconsistentAPIusage, allocator);
    route_stats.AddMember("avg_route_finalize_elapse_millis", stats_.finalizeMillis, allocator);
    route_stats.AddMember("avg_route_create_interval_seconds", stats_.avgRouteCreationInterval, allocator);
    route_stats.AddMember(
        "avg_route_segment_create_interval_seconds", stats_.avgRouteSegmentCreationInterval, allocator);
    route_stats.AddMember("num_layers", style_->getLayers().size(), allocator);
    route_stats.AddMember("num_sources", style_->getSources().size(), allocator);
    route_stats.AddMember(
        "max_route_vanishing_elapsed_millis", std::to_string(stats_.maxRouteVanishingElapsedMillis), allocator);
    route_stats.AddMember(
        "min_route_vanishing_elapsed_millis", std::to_string(stats_.minRouteVanishingElapsedMillis), allocator);
    route_stats.AddMember(
        "avg_route_vanishing_elapsed_millis", std::to_string(stats_.avgRouteVanishingElapsedMillis), allocator);
    route_stats.AddMember(
        "num_large_delta_vanishing_percents", std::to_string(stats_.numLargeDeltaVanishingPercents), allocator);

    document.AddMember("route_stats", route_stats, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    return buffer.GetString();
}

void RouteManager::finalizeRoute(const RouteID& routeID, const DirtyType& dt) {
    assert(routeID.isValid() && "invalid route ID");
    using namespace mbgl::style;
    using namespace mbgl::style::expression;

    const auto& createDynamicWidthExpression = [](const std::map<double, double>& zoomstops) {
        ParsingContext pc;
        ParseResult pr = dsl::zoom();
        std::unique_ptr<Expression> zoomValueExp = std::move(pr.value());

        Interpolator linearInterpolator = dsl::linear();

        using namespace mbgl::style::expression;
        std::map<double, std::unique_ptr<Expression>> stops;

        for (auto& zoomstopIter : zoomstops) {
            stops[zoomstopIter.first] = dsl::literal(zoomstopIter.second);
        }

        const auto& type = stops.begin()->second->getType();
        ParsingContext ctx;
        ParseResult result = createInterpolate(
            type, std::move(linearInterpolator), std::move(zoomValueExp), std::move(stops), ctx);
        assert(result);
        std::unique_ptr<Expression> expression = std::move(*result);
        PropertyExpression<float> expressionFloat = PropertyExpression<float>(std::move(expression));
        return expressionFloat;
    };

    const auto& createLayer = [&](const std::string& sourceID,
                                  const std::string& layerID,
                                  const Route& route,
                                  const Color& color,
                                  const Color& clipColor,
                                  int width,
                                  const std::map<double, double>& zoomstops) {
        if (style_->getSource(sourceID) != nullptr) {
            return false;
        }
        mapbox::geojsonvt::feature_collection featureCollection;
        const auto& geom = route.getGeometry();
        featureCollection.emplace_back(geom);

        GeoJSONOptions opts;
        opts.lineMetrics = true;
        opts.buffer = 0;
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

        layer->setGradientLineFilter(LineGradientFilterType::Nearest);
        layer->setGradientLineClipColor(clipColor);
        layer->setIsLayerUsingRoute(true);

        if (zoomstops.empty()) {
            layer->setLineWidth(width);
        } else {
            const PropertyExpression<float>& lineWidthExpression = createDynamicWidthExpression(zoomstops);
            layer->setLineWidth(lineWidthExpression);
        }

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

    std::string captureExtraDataStr;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        bool updateRouteLayers = false;
        bool updateGradients = false;
        bool updateProgress = false;
        switch (dt) {
            case DirtyType::dtRouteGeometry: {
                updateRouteLayers = true;
                updateGradients = true;
                updateProgress = true;
            } break;

            case DirtyType::dtRouteProgress: {
                updateProgress = true;
            } break;

            case DirtyType::dtRouteSegments: {
                updateGradients = true;
            } break;
        }

        std::string casingLayerName = getBaseRouteLayerName(routeID);
        std::string casingGeoJSONSourceName = getBaseGeoJSONsourceName(routeID);
        std::string activeLayerName = getActiveRouteLayerName(routeID);
        std::string activeGeoJSONSourceName = getActiveGeoJSONsourceName(routeID);
        auto& route = routeMap_.at(routeID);
        const RouteOptions& routeOptions = route.getRouteOptions();

        if (updateRouteLayers) {
            // create layer for casing/base
            std::map<double, double> baseZoomStops;
            if (routeOptions.useDynamicWidths) {
                baseZoomStops = !routeOptions.outerWidthZoomStops.empty() ? routeOptions.outerWidthZoomStops
                                                                          : getDefaultRouteLineCasingWeights();
            }
            if (!createLayer(casingGeoJSONSourceName,
                             casingLayerName,
                             route,
                             routeOptions.outerColor,
                             routeOptions.outerClipColor,
                             routeOptions.outerWidth,
                             baseZoomStops)) {
                stats_.inconsistentAPIusage = true;
                mbgl::Log::Info(mbgl::Event::Style, "Trying to update a layer that is not created");
            }

            // create layer for active/blue
            std::map<double, double> activeLineWidthStops;
            if (routeOptions.useDynamicWidths) {
                activeLineWidthStops = !routeOptions.innerWidthZoomStops.empty() ? routeOptions.innerWidthZoomStops
                                                                                 : getDefaultRouteLineWeights();
            }
            if (!createLayer(activeGeoJSONSourceName,
                             activeLayerName,
                             route,
                             routeOptions.innerColor,
                             routeOptions.innerClipColor,
                             routeOptions.innerWidth,
                             activeLineWidthStops)) {
                stats_.inconsistentAPIusage = true;
                mbgl::Log::Info(mbgl::Event::Style, "Trying to update a layer that is not created");
            }
        }

        Layer* activeRouteLayer = style_->getLayer(activeLayerName);
        LineLayer* activeRouteLineLayer = static_cast<LineLayer*>(activeRouteLayer);
        if (!activeRouteLineLayer) {
            stats_.inconsistentAPIusage = true;
            mbgl::Log::Info(mbgl::Event::Style, "Trying to update a layer that is not created");
        }
        Layer* baseRouteLayer = style_->getLayer(casingLayerName);
        LineLayer* casingRouteLineLayer = static_cast<LineLayer*>(baseRouteLayer);
        if (!casingRouteLineLayer) {
            stats_.inconsistentAPIusage = true;
            mbgl::Log::Info(mbgl::Event::Style, "Trying to update a layer that is not created");
        }

        // Create the gradient colors expressions and set on the active layer
        std::unordered_map<std::string, std::string> gradientDebugMap;
        if (updateGradients) {
            // create the gradient expression for active route.
            std::map<double, mbgl::Color> innerGradientMap = route.getRouteSegmentColorStops(RouteType::Inner,
                                                                                             routeOptions.innerColor);

            std::unique_ptr<expression::Expression> innerGradientExpression = createGradientExpression(
                innerGradientMap);

            ColorRampPropertyValue activeColorRampProp(std::move(innerGradientExpression));
            activeRouteLineLayer->setLineGradient(activeColorRampProp);

            // create the gradient expression for the base route
            std::map<double, mbgl::Color> casingLayerGradient = route.getRouteSegmentColorStops(
                RouteType::Casing, routeOptions.outerColor);
            // std::map<double, mbgl::Color> casingLayerGradient = route.getRouteColorStops(routeOptions.outerColor);
            std::unique_ptr<expression::Expression> casingLayerExpression = createGradientExpression(
                casingLayerGradient);

            ColorRampPropertyValue casingColorRampProp(std::move(casingLayerExpression));
            casingRouteLineLayer->setLineGradient(casingColorRampProp);
        }

        if (updateProgress) {
            double progress = route.routeGetProgress();
            activeRouteLineLayer->setGradientLineClip(progress);
            casingRouteLineLayer->setGradientLineClip(progress);
        }
    }
}

bool RouteManager::setVanishingRouteID(const RouteID& routeID) {
    bool success = false;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end() && style_ != nullptr) {
        vanishingRouteID_ = routeID;
        success = true;
    }

    return success;
}

RouteID RouteManager::getVanishingRouteID() const {
    return vanishingRouteID_;
}

double RouteManager::getTotalDistance(const RouteID& routeID) {
    assert(routeID.isValid() && "invalid route ID");
    double result = -1.0;
    if (routeID.isValid() && routeMap_.find(routeID) != routeMap_.end()) {
        result = routeMap_.at(routeID).getTotalDistance();
    }

    return result;
}

void RouteManager::applyEmergencyDiagnostics() {
    // check if there is inconsistent API usage (TODO: perhaps we should throw an exception rather than just logging it)
    bool logTraces = false;
    if (stats_.inconsistentAPIusage) {
        Log::Warning(Event::Route,
                     "Inconsistent API usage detected in RouteManager. The layers are being modified without complete "
                     "finalizing of its creation");
        logTraces = true;
    }
    // check if there is a very high frequent geometry change
    if (stats_.avgRouteCreationInterval > 0 && stats_.avgRouteCreationInterval < 1.0) {
        Log::Warning(Event::Route,
                     "High frequency of route geometry changes detected in RouteManager. Average interval between "
                     "route creations is " +
                         std::to_string(stats_.avgRouteCreationInterval) + " seconds");
        logTraces = true;
    }
    // check if there is a very large change in route vanishing
    if (stats_.numLargeDeltaVanishingPercents > 0) {
        Log::Warning(
            Event::Route,
            "Large delta changes in route vanishing detected in RouteManager. Number of large delta changes: " +
                std::to_string(stats_.numLargeDeltaVanishingPercents));
        logTraces = true;
    }

    if (logTraces) {
        // get route layers and 2 layers before and after
        const auto getLayersNearRoutes = [&]() -> std::vector<std::string> {
            std::vector<std::string> retLayers;

            if (style_ != nullptr) {
                std::vector<style::Layer*> layers = style_->getLayers();

                if (!layers.empty()) {
                    // Collect all route layer names
                    std::unordered_set<std::string> routeLayerNames;
                    for (const auto& routePair : routeMap_) {
                        const RouteID& routeID = routePair.first;
                        routeLayerNames.insert(getActiveRouteLayerName(routeID));
                        routeLayerNames.insert(getBaseRouteLayerName(routeID));
                    }

                    // Find the topmost and bottommost route layer indices
                    size_t topMostRouteIdx = layers.size(); // Initialize to invalid
                    size_t bottomMostRouteIdx = 0;
                    bool foundAnyRouteLayer = false;

                    for (size_t i = 0; i < layers.size(); ++i) {
                        if (routeLayerNames.find(layers[i]->getID()) != routeLayerNames.end()) {
                            if (!foundAnyRouteLayer) {
                                topMostRouteIdx = i;
                                bottomMostRouteIdx = i;
                                foundAnyRouteLayer = true;
                            } else {
                                if (i < topMostRouteIdx) topMostRouteIdx = i;
                                if (i > bottomMostRouteIdx) bottomMostRouteIdx = i;
                            }
                        }
                    }

                    if (foundAnyRouteLayer) {
                        // Calculate the range: 2 layers before top, to 2 layers after bottom
                        size_t startIdx = (topMostRouteIdx >= 2) ? (topMostRouteIdx - 2) : 0;
                        size_t endIdx = std::min(bottomMostRouteIdx + 2, layers.size() - 1);

                        // Collect all layers in this range
                        for (size_t i = startIdx; i <= endIdx; ++i) {
                            retLayers.push_back(layers[i]->getID());
                        }
                    }
                }
            }

            return retLayers;
        };

        // Get and log the layers
        std::vector<std::string> nearbyLayers = getLayersNearRoutes();

        std::string apitraces = embeddAPIcaptures(stats_.recentApiCalls);
        Log::Warning(Event::Route, "Last 100 API traces leading to the issue: ");
        Log::Warning(Event::Route, apitraces);

        // Format and log the layer information
        std::stringstream layerInfo;
        layerInfo << "[";
        for (size_t i = 0; i < nearbyLayers.size(); ++i) {
            layerInfo << "\"" << nearbyLayers[i] << "\"";
            if (i < nearbyLayers.size() - 1) {
                layerInfo << ", ";
            }
        }
        layerInfo << "]";
        Log::Warning(Event::Route, "Layers in route proximity: " + layerInfo.str());

    } else {
        Log::Info(Event::Route, "Route diagnostics applied and found no issues ");
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
            // create the layers and geojsonsource for casing route
            for (const auto& iter : dirtyRouteMap_) {
                DirtyType dirtyType = iter.first;
                for (const auto& routeID : iter.second) {
                    finalizeRoute(routeID, dirtyType);
                }
            }
            dirtyRouteMap_.clear();
        }
    }
    auto stopclock = high_resolution_clock::now();
    stats_.finalizeMillis = duration_cast<milliseconds>(stopclock - startclock).count();

    TRACE_ROUTE_CALL(stats_.recentApiCalls, {}, "void", "", {}, "");
}

RouteManager::~RouteManager() {}

} // namespace route
} // namespace mbgl
