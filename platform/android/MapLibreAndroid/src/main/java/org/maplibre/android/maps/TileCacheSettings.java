package org.maplibre.android.maps;

final public class TileCacheSettings {
    /***
     * The tile source ID
     */
    public String source;

    /***
     * The computed tile cache size is clamped between minTiles and maxTiles
     */
    public int minTiles;

    /***
     * The computed tile cache size is clamped between minTiles and maxTiles
     */
    public int maxTiles;

    /***
     * Zoom below minZoom will not be cached
     */
    public int minZoom;

    /***
     * Zoom above maxZoom will not be cached
     */
    public int maxZoom;

    /***
     * The maximum age of cached tiles in seconds
     * If 0, no tiles are removed based on age
     */
    public double cachedTileMaxAge;

    /***
     * The maximum age of tiles being rendered in seconds.
     * If 0, no render tiles are removed based on age.
     * The same render tiles are reused when the camera is not moving.
     * Setting a maximum age ensures short lived tiles such as rendered
     * traffic tiles are invalidated which forces a cache request.
     */
    public double renderTileMaxAge;

    /***
     * If true, tiles that are not ideal are also cached
     * Check update_renderables.hpp for details about ideal tiles
     */
    public boolean aggressiveCache;

    /***
     * For certain sources, it is possible to not clear the cache when only
     * geojson causes a tile relayout
     */
    public boolean skipRelayoutClear;
}
