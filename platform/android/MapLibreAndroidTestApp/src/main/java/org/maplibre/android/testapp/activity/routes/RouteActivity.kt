package org.maplibre.android.testapp.activity.routes

import android.content.Context
import android.location.LocationManager
import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.SeekBar
import android.widget.Spinner
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import org.maplibre.android.camera.CameraPosition
import org.maplibre.android.geometry.LatLng
import org.maplibre.android.location.LocationComponentActivationOptions
import org.maplibre.android.location.LocationComponentOptions
import org.maplibre.android.location.modes.CameraMode
import org.maplibre.android.location.modes.RenderMode
import org.maplibre.android.location.permissions.PermissionsListener
import org.maplibre.android.location.permissions.PermissionsManager
import org.maplibre.android.maps.MapLibreMap
import org.maplibre.android.maps.MapView
import org.maplibre.android.maps.OnMapReadyCallback
import org.maplibre.android.maps.Style
import org.maplibre.android.testapp.R
import org.maplibre.android.testapp.utils.ApiKeyUtils

class RouteActivity : AppCompatActivity(), OnMapReadyCallback {
    private lateinit var mapView: MapView
    private lateinit var maplibreMap : MapLibreMap
    private var progressModePoint : Boolean = false
    private var progressPrecisionCoarse : Boolean = false

    private val useLocationEngine = false
    private var permissionsManager: PermissionsManager? = null
    private var locationManager : LocationManager? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        checkPermissions()

        setContentView(R.layout.activity_routes)
        mapView = findViewById(R.id.mapView)
        mapView.onCreate(savedInstanceState)
        mapView.getMapAsync(this)

        //Add route
        val addRouteButton = findViewById<Button>(R.id.add_route)
        addRouteButton?.setOnClickListener {
            RouteUtils.addRoute(mapView)

            Toast.makeText(this, "Added Route", Toast.LENGTH_SHORT).show()
        }

        //Remove route
        val removeRouteButton = findViewById<Button>(R.id.remove_route)
        removeRouteButton?.setOnClickListener {
            RouteUtils.disposeRoute(mapView)
            Toast.makeText(this, "Removed Route", Toast.LENGTH_SHORT).show()
        }


        //route progress mode
        val progressModeSpinner = findViewById<Spinner>(R.id.route_progress_mode)
        val progressModeOptions = listOf("Point", "Percent")
        val progressModeAdapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, progressModeOptions)
        progressModeAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        progressModeSpinner.adapter = progressModeAdapter

        // Set a listener for the progress mode spinner
        progressModeSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
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

        //route progress precision
        val progressPrecisionSpinner = findViewById<Spinner>(R.id.route_progress_precision)
        val progressPrecisionOptions = listOf("Coarse", "Fine")
        val progressPrecisionAdapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, progressPrecisionOptions)
        progressPrecisionAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        progressPrecisionSpinner.adapter = progressPrecisionAdapter

        // Set a listener for the progress precision spinner
        progressPrecisionSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                // Get the selected item
                val selectedItem = parent?.getItemAtPosition(position).toString()
                // Handle the selected item
                when (selectedItem) {
                    "Coarse" -> {
                        // Handle percent selection
                        progressPrecisionCoarse = true
                    }
                    "Fine" -> {
                        // Handle point selection
                        progressPrecisionCoarse = false
                    }
                }
                Toast.makeText(applicationContext, "Selected: $selectedItem", Toast.LENGTH_SHORT).show()
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {
                Toast.makeText(applicationContext, "Nothing selected", Toast.LENGTH_SHORT).show()
            }
        }

        //route progress slider
        val sliderBar = findViewById<SeekBar>(R.id.route_slider)
        sliderBar?.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                var progressPercent = progress.toDouble() / 100.0

                if(progressModePoint) {
                    RouteUtils.setPointProgress(mapView, progressPercent, progressPrecisionCoarse)
                } else {
                    RouteUtils.setPercentProgress(mapView, progressPercent)
                }

                mapView.finalizeRoutes()
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

    }

    private fun prepareLocationComp(style: Style) {
        val context : Context = this
        val locationComponentOptions =
            LocationComponentOptions.builder(context)
                .compassAnimationEnabled(true)
                .gpsDrawable(R.drawable.ic_puck)
                .foregroundDrawableStale(R.drawable.ic_puck)
                .foregroundDrawable(R.drawable.ic_puck)
                .backgroundDrawable(R.drawable.ic_transparent)
                .bearingDrawable(R.drawable.ic_transparent)
                .trackingAnimationDurationMultiplier(1.0f)
                .build()

        maplibreMap.locationComponent.apply {
            activateLocationComponent(
                LocationComponentActivationOptions.Builder(
                    context,
                    style
                ).locationComponentOptions(locationComponentOptions)
                    .useDefaultLocationEngine(useLocationEngine)
                    .customPuckAnimationEnabled(true)
                    .customPuckAnimationIntervalMS(30)
                    .customPuckLagMS(900)
                    .customPuckIconScale(0.8f)
                    .build()
            )
            applyStyle(locationComponentOptions)
//            isLocationComponentEnabled = true
            renderMode = RenderMode.GPS
            cameraMode = CameraMode.TRACKING_GPS
        }
        maplibreMap.locationComponent.setMaxAnimationFps(30)
        if (useLocationEngine) {
            maplibreMap.cameraPosition = CameraPosition.Builder().target(LatLng(0.0,0.0)).zoom(16.4).build()
            maplibreMap.locationComponent.setCameraMode(CameraMode.TRACKING_GPS)
        } else {
            maplibreMap.locationComponent.zoomWhileTracking(16.4, 1000)
        }
    }


    private fun checkPermissions() {
        val context : Context = this

        if (PermissionsManager.areLocationPermissionsGranted(this)) {
            // mapView.getMapAsync(this)
        } else {
            permissionsManager = PermissionsManager(object : PermissionsListener {
                override fun onExplanationNeeded(permissionsToExplain: List<String>) {
                    Toast.makeText(
                        context,
                        "You need to accept location permissions.",
                        Toast.LENGTH_SHORT
                    ).show()
                }

                override fun onPermissionResult(granted: Boolean) {
                    if (granted) {
                        // mapView.getMapAsync(this@BasicLocationPulsingCircleActivity)
                    } else {
                        finish()
                    }
                }
            })
            permissionsManager!!.requestLocationPermissions(this)
        }
    }

    override fun onMapReady(map: MapLibreMap) {
        // Set up the map and add route data here
        maplibreMap = map
        val key = ApiKeyUtils.getApiKey(applicationContext)
        if (key == null || key == "YOUR_API_KEY_GOES_HERE") {
            maplibreMap.setStyle(
                Style.Builder().fromUri("https://demotiles.maplibre.org/style.json")
            ) {
                style: Style ->
                prepareLocationComp(style)
            }
        } else {
            val styles = Style.getPredefinedStyles()
            if (styles.isNotEmpty()) {
                val styleUrl = styles[0].url
                maplibreMap.setStyle(Style.Builder().fromUri(styleUrl))
                maplibreMap.getStyle() { style ->
                    prepareLocationComp(style)
                }
            }
        }

        //TODO: investigate and fix location component to programmatically set puck location
//        val location : Location = Location("dummyprovider")
//        location.latitude = 0.0
//        location.longitude = 0.0
//        maplibreMap.locationComponent.forceLocationUpdate(location)

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
