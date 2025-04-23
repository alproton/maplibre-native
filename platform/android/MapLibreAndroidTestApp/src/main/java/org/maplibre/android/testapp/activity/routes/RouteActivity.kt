package org.maplibre.android.testapp.activity.routes

import android.graphics.Color
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import org.maplibre.android.maps.MapLibreMap
import org.maplibre.android.maps.MapView
import org.maplibre.android.maps.OnMapReadyCallback
import org.maplibre.android.maps.RouteID
import org.maplibre.android.maps.RouteOptions
import org.maplibre.android.maps.RouteSegmentOptions
import org.maplibre.android.maps.Style
import org.maplibre.android.testapp.R
import org.maplibre.android.testapp.utils.ApiKeyUtils
import org.maplibre.geojson.LineString
import org.maplibre.geojson.Point

class RouteActivity : AppCompatActivity(), OnMapReadyCallback {
    private lateinit var mapView: MapView
    private var routeID : RouteID = RouteID(0);
    private val DELAY_SECONDS = 3L // Delay in seconds
    private val DELAY_MILLISECONDS = DELAY_SECONDS * 1000 // Convert to milliseconds
    private lateinit var maplibreMap : MapLibreMap

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_routes)
        mapView = findViewById(R.id.mapView)
        mapView.onCreate(savedInstanceState)
        mapView.getMapAsync(this)

        mapView.addOnDidFinishLoadingStyleListener {
            if(this::maplibreMap.isInitialized) {
                setupRoutes()
            }
        }
    }

    fun setupRoutes() {
        val route_resolution = 50;
        val radius : Double = 50.0
        val route_geometry : LineString
        val points = mutableListOf<Point>()
        for (i in 0..route_resolution-1) {
            val anglerad : Double = (i.toDouble() / (route_resolution-1).toDouble()) * 2.0 * Math.PI
            val pt : Point = Point.fromLngLat(radius * Math.sin(anglerad),
                radius * Math.cos(anglerad))
            points.add(pt)
        }

        route_geometry = LineString.fromLngLats(points)

        var route_options = RouteOptions();
        route_options.innerWidth = 14.0
        route_options.outerWidth = 16.0
        route_options.innerColor = Color.argb(255, 0, 0, 255)
        route_options.outerColor = Color.argb(255, 0, 255, 0)

        routeID = mapView.createRoute(route_geometry, route_options)

        var route_segment_geom = mutableListOf<Point>()
        for(i in 0..(route_resolution-1)/2) {
            val anglerad : Double = (i.toDouble() / (route_resolution-1).toDouble()) * 2.0 * Math.PI
            val pt : Point = Point.fromLngLat(radius * Math.sin(anglerad),
                radius * Math.cos(anglerad))
            route_segment_geom.add(pt)
        }
        var rsopts = RouteSegmentOptions()
        rsopts.color = Color.argb(255, 120, 0, 0)
        rsopts.outerColor = Color.argb(255, 0, 122, 0)
        rsopts.priority = 0
        rsopts.geometry = LineString.fromLngLats(route_segment_geom)
        mapView.createRouteSegment(routeID, rsopts)

        mapView.finalizeRoutes()
    }

    override fun onMapReady(map: MapLibreMap) {
        // Set up the map and add route data here
        maplibreMap = map
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


//        lifecycleScope.launch {
//            delay(DELAY_MILLISECONDS) // Delay before executing the code
//
//        }
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
