package org.maplibre.android.testapp.activity.style

import android.os.Bundle
import androidx.annotation.IntegerRes
import androidx.appcompat.app.AppCompatActivity
import okhttp3.Route
import org.maplibre.android.maps.MapLibreMap
import org.maplibre.android.maps.MapView
import org.maplibre.android.maps.OnMapReadyCallback
import org.maplibre.android.maps.RouteID
import org.maplibre.android.maps.RouteOptions
import org.maplibre.android.maps.Style
import org.maplibre.android.testapp.R
import org.maplibre.android.testapp.utils.ApiKeyUtils
import org.maplibre.geojson.LineString
import org.maplibre.geojson.Point

class RouteActivity : AppCompatActivity(), OnMapReadyCallback {
    private lateinit var mapView: MapView
    private var routeID : RouteID = RouteID(0);

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_routes)
        mapView = findViewById(R.id.mapView)
        mapView.onCreate(savedInstanceState)
        mapView.getMapAsync(this)
    }

    override fun onMapReady(maplibreMap: MapLibreMap) {
        // Set up the map and add route data here
        val key = ApiKeyUtils.getApiKey(applicationContext)
        if (key == null || key == "YOUR_API_KEY_GOES_HERE") {
            maplibreMap.setStyle(
                Style.Builder().fromUri("https://demotiles.maplibre.org/style.json")
            )
        } else {
            val styles = Style.getPredefinedStyles()
            if (styles.isNotEmpty()) {
                val styleUrl = styles[0].url
                maplibreMap.setStyle(Style.Builder().fromUri(styleUrl))
            }
        }

        val route_resolution = 50;
        val radius : Double = 5.0
        val route_geometry : LineString
        val points = mutableListOf<Point>()
        for (i in 0..route_resolution) {
            val anglerad : Double = (i / route_resolution) * 2.0 * Math.PI

            
            val pt : Point = Point.fromLngLat(radius * Math.sin(anglerad),
                                                    radius * Math.cos(anglerad))
            points.add(pt)
        }

        route_geometry = LineString.fromLngLats(points)

        var route_options : RouteOptions = RouteOptions();
        route_options.innerWidth = 14.0
        route_options.outerWidth = 16.0
        route_options.innerColor = 0xFF0000
        route_options.outerColor = 0x00FF00

        routeID = mapView.createRoute(route_geometry, route_options)
        mapView.finalizeRoutes()

    }

    override fun onStart() {
        super.onStart()
        mapView.onStart()
    }

    override fun onResume() {
        super.onResume()
        mapView.onResume()
    }

    override fun onPause() {
        super.onPause()
        mapView.onPause()
    }

    override fun onStop() {
        super.onStop()
        mapView.onStop()
    }

    public override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        mapView.onSaveInstanceState(outState)
    }

    override fun onLowMemory() {
        super.onLowMemory()
        mapView.onLowMemory()
    }

    public override fun onDestroy() {
        super.onDestroy()
        mapView.onDestroy()
    }
}
