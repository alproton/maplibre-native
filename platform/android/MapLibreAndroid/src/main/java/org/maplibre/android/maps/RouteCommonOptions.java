package org.maplibre.android.maps;

final public class RouteCommonOptions {
    public int outerColor;
    public int innerColor;
    double outerWidth = 10;
    double innerWidth = 6;
    //experimental for now, until a default native solution is implemented
    double segmentTransitionDist = 1e-3;
}
