#pragma once

#include <string>
#include <unordered_map>

namespace mbgl {

struct TileCacheSettings {
    std::string source;           // The tile source ID
    int minTiles = 0;             // The computed tile cache size is clamped between minTiles and maxTiles
    int maxTiles = 0;             // The computed tile cache size is clamped between minTiles and maxTiles
    bool aggressiveCache = false; // If true, tiles that are not ideal are also cached. Check update_renderables.hpp for
                                  // details about ideal tiles.
};

using TileCacheSettingsMap = std::unordered_map<std::string, TileCacheSettings>;

} // namespace mbgl
