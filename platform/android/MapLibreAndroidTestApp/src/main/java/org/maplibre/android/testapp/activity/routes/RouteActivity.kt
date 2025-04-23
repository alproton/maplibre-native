package org.maplibre.android.testapp.activity.routes

import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.SeekBar
import android.widget.Spinner
import android.widget.Toast
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
    private lateinit var maplibreMap : MapLibreMap
    private var progressModePoint : Boolean = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_routes)
        mapView = findViewById(R.id.mapView)
        mapView.onCreate(savedInstanceState)
        mapView.getMapAsync(this)

        val addRouteButton = findViewById<Button>(R.id.add_route)
        addRouteButton?.setOnClickListener {
            RouteUtils.addRoute(mapView)

            Toast.makeText(this, "Added Route", Toast.LENGTH_SHORT).show()
        }

        val removeRouteButton = findViewById<Button>(R.id.remove_route)
        removeRouteButton?.setOnClickListener {
            RouteUtils.disposeRoute(mapView)
            Toast.makeText(this, "Removed Route", Toast.LENGTH_SHORT).show()
        }

        val sliderBar = findViewById<SeekBar>(R.id.route_slider)
        sliderBar?.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                var progressPercent = progress.toDouble() / 100.0

                if(progressModePoint) {
                    RouteUtils.setPointProgress(mapView, progressPercent)
                } else {
                    RouteUtils.setPercentProgress(mapView, progressPercent)
                }

                mapView.finalizeRoutes()
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

        val spinner = findViewById<Spinner>(R.id.route_progress_mode)
        val progressModeOptions = listOf("Percent", "Point")
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, progressModeOptions)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        spinner.adapter = adapter

        // Set a listener for the Spinner
        spinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                // Get the selected item
                val selectedItem = parent?.getItemAtPosition(position).toString()
                // Handle the selected item
                when (selectedItem) {
                    "Percent" -> {
                        // Handle percent selection
                        progressModePoint = false
                    }
                    "Point" -> {
                        // Handle point selection
                        progressModePoint = true
                    }
                }
                Toast.makeText(applicationContext, "Selected: $selectedItem", Toast.LENGTH_SHORT).show()
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {
                Toast.makeText(applicationContext, "Nothing selected", Toast.LENGTH_SHORT).show()
            }
        }

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
