package org.maplibre.android.maps;

final public class RouteCommonOptions {
    public int outerColor;
    public int innerColor;
    public double outerWidth = 10;
    public double innerWidth = 6;
    //experimental for now, until a default native solution is implemented
    public double segmentTransitionDist = 1e-6;
}
