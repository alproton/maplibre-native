#pragma once

#include <string>
#include <unordered_map>

namespace mbgl {

struct TileCacheSettings {
    std::string source;          // The tile source ID
    int minTiles = 0;            // The computed tile cache size is clamped between minTiles and maxTiles
    int maxTiles = 0;            // The computed tile cache size is clamped between minTiles and maxTiles
    int minZoom = 0;             // The minimum zoom level for the tile cache
    int maxZoom = 32;            // The maximum zoom level for the tile cache
    double cachedTileMaxAge = 0; // The maximum age of cached tiles in seconds. If 0, no tiles are removed based on age
    double renderTileMaxAge =
        0; // The maximum age of tiles being rendered in seconds. If 0, no render tiles are removed based on age. The
           // same render tiles are reused when the camera is not moving. Setting a maximum age ensures short lived
           // tiles such as rendered traffic tiles are invalidated which forces a cache request.
    bool aggressiveCache = false; // If true, tiles that are not ideal are also cached. Check update_renderables.hpp for
                                  // details about ideal tiles.
    bool skipRelayoutClear =
        false; // For certain sources, it is possible to not clear the cache when only geojson causes a tile relayout
};

using TileCacheSettingsMap = std::unordered_map<std::string, TileCacheSettings>;

} // namespace mbgl
