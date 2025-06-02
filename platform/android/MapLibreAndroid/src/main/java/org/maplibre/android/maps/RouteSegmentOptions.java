package org.maplibre.android.maps;

import org.maplibre.geojson.LineString;

final public class RouteSegmentOptions {
    public LineString geometry;
    public int firstIndex = -1;
    public int lastIndex = -1;
    public float firstIndexFraction = 0.0f;
    public float lastIndexFraction = 0.0f;
    public int color;
    public int outerColor;
    public int priority = 0;
}
