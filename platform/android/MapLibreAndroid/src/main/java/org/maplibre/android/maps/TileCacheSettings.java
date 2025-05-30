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
