package org.maplibre.android.testapp.activity.routes

import android.content.Context
import android.location.Location
import android.location.LocationListener
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
import org.maplibre.geojson.Point
import timber.log.Timber

class RouteActivity : AppCompatActivity(), OnMapReadyCallback {
    private lateinit var mapView: MapView
    private lateinit var maplibreMap : MapLibreMap
    private var progressPrecisionCoarse : Boolean = true
    private val enableAutoVanishingRoute = false
    private val useFractionalRouteSegments = true
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
        mapView.setAutoVanishingRoute(enableAutoVanishingRoute)
        mapView.enableCaptureRouteNavStops(false)
        //Add route
        val addRouteButton = findViewById<Button>(R.id.add_route)
        addRouteButton?.setOnClickListener {
            RouteUtils.addRoute(mapView, useFractionalRouteSegments)
            mapView.applyRouteDiagnostics()

            Toast.makeText(this, "Added Route", Toast.LENGTH_SHORT).show()
        }

        //Remove route
        val removeRouteButton = findViewById<Button>(R.id.remove_route)
        removeRouteButton?.setOnClickListener {
            RouteUtils.disposeFirstRoute(mapView)
            Toast.makeText(this, "Removed Route", Toast.LENGTH_SHORT).show()
        }

        val logRouteSnapshotButton = findViewById<Button>(R.id.log_route_snapshot)
        logRouteSnapshotButton?.setOnClickListener {
            val snapshot = mapView.getSnapshotCapture()
            Timber.tag("ROUTE_PROGRESS").i("Route snapshot: $snapshot")
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
                val progressPercent = progress.toDouble() / 100.0
                if(RouteUtils.isCaptureLoaded()) {
                    RouteUtils.scrubCaptureRoute(mapView, progressPercent)
                } else {
                    val pt : Point = RouteUtils.getPointProgress(progressPercent)

                    if(!enableAutoVanishingRoute) {
                        RouteUtils.setPointProgress(mapView, progressPercent, progressPrecisionCoarse)
                        mapView.finalizeRoutes()
                    }

                    val location = Location(LocationManager.GPS_PROVIDER)
                    location.latitude = pt.latitude()
                    location.longitude = pt.longitude()
                    maplibreMap.locationComponent.forceLocationUpdate(location)

                    Timber.d("#####Set######## " + location.latitude.toString() + "  ,  " + location.longitude.toString())
                    mapView.finalizeRoutes()
                }
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
                .gpsDrawable(R.drawable.ic_my_location)
                .foregroundDrawableStale(R.drawable.ic_my_location)
                .foregroundDrawable(R.drawable.ic_my_location)
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

    private val locationListener: LocationListener = object : LocationListener {
        override fun onLocationChanged(location: Location) {
            if (!useLocationEngine) {
                maplibreMap.locationComponent.forceLocationUpdate(location)
            }

            Timber.d("##################### " + location.latitude.toString() + "  ,  " + location.longitude.toString())
        }
        override fun onStatusChanged(provider: String, status: Int, extras: Bundle) {}
        override fun onProviderEnabled(provider: String) {}
        override fun onProviderDisabled(provider: String) {}
    }

    private fun prepareLocationManager() {
        val context : Context = this
        locationManager = getSystemService(LOCATION_SERVICE) as LocationManager?
        try {
            // Request location updates
            locationManager?.requestLocationUpdates(LocationManager.GPS_PROVIDER, 1L, 1f, locationListener)
        } catch(ex: SecurityException) {
            Timber.d("######################### Security Exception, no location available")
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

        prepareLocationManager()

        //capture
        val captureSpinner = findViewById<Spinner>(R.id.capture_entries)
        val captureOptions = listOf("yosemite")
        val captureAdapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, captureOptions)
        captureAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        captureSpinner.adapter = captureAdapter

        // Set a listener for the progress precision spinner
        captureSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                // Get the selected item
                val selectedItem = parent?.getItemAtPosition(position).toString()
                // Handle the selected item
                if(!RouteUtils.isCaptureLoaded()) {
                    val routeCaptureSample = RouteUtils.readJsonFromRaw(applicationContext, R.raw.yosemite_route_capture)
                    RouteUtils.loadCapture(mapView, routeCaptureSample!!)

                    Toast.makeText(applicationContext, "Selected: $selectedItem", Toast.LENGTH_SHORT).show()
                }
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {
                Toast.makeText(applicationContext, "Nothing selected", Toast.LENGTH_SHORT).show()
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
