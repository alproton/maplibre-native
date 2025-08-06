#include "route_pick_test.hpp"
#include "../glfw_view.hpp"
#include "../glfw_renderer_frontend.hpp"

RoutePickTest::RoutePickTest(const std::string& testDir)
    : RouteTest("route_pick_test", testDir) {}

bool RoutePickTest::pickRoute(GLFWView* view, double x, double y) {
    pickedRouteID = RouteID();
    std::vector<RouteID> routeIDs = rmptr_->getAllRoutes();
    std::vector<std::string> layers;
    // we specifically create caches for base layer since base route is wider than the active layer.
    // we also check against source name as well as source layer name, since I've seen cases where the source layer
    // name is not set.
    std::unordered_map<std::string, RouteID> baseLayerMapCache;
    std::unordered_map<std::string, RouteID> baseSourceMapCache;
    for (const auto& routeID : routeIDs) {
        if (routeID.isValid()) {
            std::string baseLayer = rmptr_->getBaseRouteLayerName(routeID);
            assert(!baseLayer.empty() && "base layer cannot be empty!");
            if (!baseLayer.empty()) {
                layers.push_back(baseLayer);
                baseLayerMapCache[baseLayer] = routeID;
            }
            std::string baseSource = rmptr_->getBaseGeoJSONsourceName(routeID);
            assert(!baseSource.empty() && "base source cannot be empty");
            if (!baseSource.empty()) {
                baseSourceMapCache[baseSource] = routeID;
            }
        }
    }

    // pre-determined screen space coordinates
    int radius = 5;
    std::unordered_map<RouteID, int, IDHasher<RouteID>> routeCoverage;
    GLFWRendererFrontend* frontend = view->getRenderFrontend();
    // sample multiple ray picks over an radius
    for (int i = -radius; i < radius; i++) {
        for (int j = -radius; j < radius; j++) {
            mapbox::geometry::point<double> screenpoint = {x + i, y + j};
            std::vector<mbgl::Feature> features = frontend->queryFeatures(screenpoint.x, screenpoint.y);
            for (const auto& feature : features) {
                if (baseLayerMapCache.find(feature.sourceLayer) != baseLayerMapCache.end()) {
                    RouteID baseRouteID = baseLayerMapCache[feature.sourceLayer];
                    routeCoverage[baseRouteID]++;
                }

                // also check cache of geojson source names if the source layer is not set.
                if (baseSourceMapCache.find(feature.source) != baseSourceMapCache.end()) {
                    RouteID baseRouteID = baseSourceMapCache[feature.source];
                    routeCoverage[baseRouteID]++;
                }
            }
        }
    }

    // when you do a touch at a location, the radius can cover multiple routes.
    // find the RouteID that has the maximum touch weight value
    int maxTouchWeight = 0;
    std::vector<RouteID> maxRouteIDs;
    for (const auto& [routeID, weight] : routeCoverage) {
        if (weight > maxTouchWeight) {
            maxTouchWeight = weight;
        }
    }
    for (const auto& [routeID, weight] : routeCoverage) {
        if (weight == maxTouchWeight) {
            maxRouteIDs.push_back(routeID);
        }
    }

    if (!maxRouteIDs.empty()) {
        int top = rmptr_->getTopMost(maxRouteIDs);
        if (top >= 0 && top < static_cast<int>(maxRouteIDs.size())) {
            pickedRouteID = maxRouteIDs[top];
        }
    }

    return pickedRouteID.isValid();
}

bool RoutePickTest::produceTestCommands(mbgl::Map* map, GLFWView* view) {
    assert(map != nullptr && "invalid map!");
    assert(view != nullptr && "invalid view!");

    if (map) {
        testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
            using namespace mbgl::route;
            using namespace route_fixtures;
            map->setDebug(mbgl::MapDebugOptions::NoDebug);
            map->jumpTo(mbgl::CameraOptions().withCenter(mbgl::LatLng{}).withZoom(0).withBearing(0.0).withPitch(0.0));

            mbgl::Color color0 = routeColorTable.at(RouteColorType::RouteMapColor);
            mbgl::Color color1 = routeColorTable.at(RouteColorType::RouteMapAlternative);
            std::vector<mbgl::Color> colors = {color0, color1};

            auto getRouteGeom = [](float radius, int resolution, float xlate) -> mbgl::LineString<double> {
                mbgl::LineString<double> linestring;
                for (int i = 0; i < resolution; i++) {
                    float anglerad = (float(i) / float(resolution - 1)) * 2 * 3.14f;
                    mbgl::Point<double> pt{xlate + radius * sin(anglerad), radius * cos(anglerad)};
                    linestring.push_back(pt);
                }

                return linestring;
            };

            RouteData rd;
            rd.resolution = 10;
            rd.xlate = 0.0;
            rd.radius = 50.0;
            rd.points = getRouteGeom(rd.radius, rd.resolution, rd.xlate);
            RouteOptions routeOpts;
            routeOpts.innerColor = color0;
            routeOpts.outerColor = mbgl::Color(0.2, 0.2, 0.2, 1);
            routeOpts.useDynamicWidths = false;
            routeOpts.outerClipColor = mbgl::Color(0.5, 0.5, 0.5, 1.0);
            routeOpts.innerClipColor = mbgl::Color(0.5, 0.5, 0.5, 1.0);
            routeOpts.useMercatorProjection = false;

            const RouteID routeID = rmptr_->routeCreate(rd.points, routeOpts);
            assert(routeID.isValid() && "invalid route ID created");
            rmptr_->finalize();
            setVanishingRouteID(routeID);
            routeMap_[routeID] = rd;
        });
    }

    testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
        double screenSpaceX = 416, screenSpaceY = 412;
        bool picked = pickRoute(view, screenSpaceX, screenSpaceY);
        std::string pickedStr = picked ? "true" : "false";
        mbgl::Log::Info(mbgl::Event::Route, "Route is " + pickedStr);
        using namespace route_fixtures;
        mbgl::route::RouteSegmentOptions rsegopts;
        if (picked && pickedRouteID.isValid()) {
            rsegopts.color = mbgl::Color(0.0f, 1.0f, 0.0f, 1.0f);
        } else {
            rsegopts.color = mbgl::Color(1.0f, 0.0f, 0.0f, 1.0f);
        }

        const RouteID& routeID = routeMap_.begin()->first;
        const RouteData& rd = routeMap_.begin()->second;
        uint32_t pointSz = rd.points.size();
        rsegopts.firstIndex = 0;
        rsegopts.firstIndexFraction = 0;
        rsegopts.lastIndex = pointSz - 1;
        rsegopts.lastIndexFraction = 1.0;
        rsegopts.priority = 0;
        rsegopts.outerColor = mbgl::Color(0.0, 0.0, 0.0, 1.0);
        rmptr_->routeClearSegments(routeID);
        const bool success = rmptr_->routeSegmentCreate(routeID, rsegopts);
        rmptr_->finalize();
        assert(success && "failed to create route segment");
    });

    testCommands_.push([&]([[maybe_unused]] mbgl::Map* map, [[maybe_unused]] GLFWView* view) {
        double screenSpaceX = 0, screenSpaceY = 0;
        bool picked = pickRoute(view, screenSpaceX, screenSpaceY);
        std::string pickedStr = picked ? "true" : "false";
        mbgl::Log::Info(mbgl::Event::Route, "Route is " + pickedStr);
        using namespace route_fixtures;
        mbgl::route::RouteSegmentOptions rsegopts;
        if (pickedRouteID.isValid()) {
            rsegopts.color = mbgl::Color(0.0f, 1.0f, 0.0f, 1.0f);
        } else {
            rsegopts.color = mbgl::Color(1.0f, 0.0f, 0.0f, 1.0f);
        }

        const RouteID& routeID = routeMap_.begin()->first;
        const RouteData& rd = routeMap_.begin()->second;
        uint32_t pointSz = rd.points.size();
        rsegopts.firstIndex = 0;
        rsegopts.firstIndexFraction = 0;
        rsegopts.lastIndex = pointSz - 1;
        rsegopts.lastIndexFraction = 1.0;
        rsegopts.priority = 0;
        rsegopts.outerColor = mbgl::Color(0.0, 0.0, 0.0, 1.0);
        rmptr_->routeClearSegments(routeID);
        const bool success = rmptr_->routeSegmentCreate(routeID, rsegopts);
        rmptr_->finalize();
        assert(success && "failed to create route segment");
    });

    return true;
}

RoutePickTest::~RoutePickTest() {}
