package org.maplibre.android.testapp.activity.maplayout

import android.annotation.SuppressLint
import android.content.Context
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.os.Bundle
import android.view.*
import android.widget.*
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.drawerlayout.widget.DrawerLayout
import com.google.android.material.floatingactionbutton.FloatingActionButton
import org.maplibre.android.maps.*
import org.maplibre.android.maps.MapLibreMap.OnCameraMoveListener
import org.maplibre.android.maps.MapLibreMap.OnFpsChangedListener
import org.maplibre.android.maps.renderer.MapRenderer
import org.maplibre.android.style.layers.Layer
import org.maplibre.android.style.layers.Property
import org.maplibre.android.style.layers.PropertyFactory
import org.maplibre.android.testapp.R
import org.maplibre.android.testapp.styles.TestStyles
import timber.log.Timber
import java.util.*

import org.maplibre.android.camera.CameraPosition
import org.maplibre.android.geometry.LatLng
import org.maplibre.android.location.LocationComponentActivationOptions
import org.maplibre.android.location.LocationComponentOptions
import org.maplibre.android.location.modes.CameraMode
import org.maplibre.android.location.modes.RenderMode
import org.maplibre.android.location.permissions.PermissionsListener
import org.maplibre.android.location.permissions.PermissionsManager

/**
 * Test activity showcasing the different debug modes and allows to cycle between the default map styles.
 */
open class DebugModeActivity : AppCompatActivity(), OnMapReadyCallback, OnFpsChangedListener {
    private lateinit var mapView: MapView
    private lateinit var maplibreMap: MapLibreMap
    private var cameraMoveListener: OnCameraMoveListener? = null
    private var actionBarDrawerToggle: ActionBarDrawerToggle? = null
    private var currentStyleIndex = 0
    private var isReportFps = true
    private var isContinuousRendering = false
    private var fpsView: TextView? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        checkPermissions()

        setContentView(R.layout.activity_debug_mode)
        setupToolbar()
        setupMapView(savedInstanceState)
        setupDebugChangeView()
        setupStyleChangeView()
    }

    private fun setupToolbar() {
        val actionBar = supportActionBar
        if (actionBar != null) {
            supportActionBar!!.setDisplayHomeAsUpEnabled(true)
            supportActionBar!!.setHomeButtonEnabled(true)
            val drawerLayout = findViewById<DrawerLayout>(R.id.drawer_layout)
            actionBarDrawerToggle = ActionBarDrawerToggle(
                this,
                drawerLayout,
                R.string.navigation_drawer_open,
                R.string.navigation_drawer_close
            )
            actionBarDrawerToggle!!.isDrawerIndicatorEnabled = true
            actionBarDrawerToggle!!.syncState()
        }
    }

    private fun setupMapView(savedInstanceState: Bundle?) {
        val maplibreMapOptions = setupMapLibreMapOptions()
        mapView = MapView(this, maplibreMapOptions)
        (findViewById<View>(R.id.coordinator_layout) as ViewGroup).addView(mapView, 0)
        mapView.addOnDidFinishLoadingStyleListener {
            if (this::maplibreMap.isInitialized) {
                setupNavigationView(maplibreMap.style!!.layers)
            }
        }
        mapView.tag = true
        mapView.onCreate(savedInstanceState)
        mapView.getMapAsync(this)
        mapView.addOnDidFinishLoadingStyleListener { Timber.d("Style loaded") }
    }

    protected open fun setupMapLibreMapOptions(): MapLibreMapOptions {
        return MapLibreMapOptions.createFromAttributes(this, null)
    }

    @SuppressLint("MissingPermission")
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
            isLocationComponentEnabled = true
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

        mapView.setPuckStyle("puck_style.json")
        mapView.setPuckVariant("day")
        mapView.setPuckIconState("DefaultState")
    }

    private val useLocationEngine = false
    private var permissionsManager: PermissionsManager? = null
    private var locationManager : LocationManager? = null

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

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        permissionsManager!!.onRequestPermissionsResult(requestCode, permissions, grantResults)
    }

    private fun prepareLocationManager() {
        val context : Context = this
        locationManager = getSystemService(LOCATION_SERVICE) as LocationManager?
        try {
            // Request location updates
            locationManager?.requestLocationUpdates(LocationManager.GPS_PROVIDER, 500L, 1f, locationListener)
        } catch(ex: SecurityException) {
            Timber.d("######################### Security Exception, no location available")
        }
    }

    override fun onMapReady(map: MapLibreMap) {
        maplibreMap = map
        maplibreMap.setStyle(
            Style.Builder().fromUri(STYLES[currentStyleIndex])
        ) { style: Style ->
            prepareLocationComp(style)
            setupNavigationView(style.layers)
        }
        setupZoomView()
        setFpsView()

        if (!useLocationEngine) {
            prepareLocationManager()
        }
    }

    private fun setFpsView() {
        fpsView = findViewById(R.id.fpsView)
        maplibreMap.setOnFpsChangedListener(this)
    }

    override fun onFpsChanged(fps: Double) {
        fpsView!!.text = String.format(Locale.US, "FPS: %4.2f", fps)
    }

    private fun setupNavigationView(layerList: List<Layer>) {
        Timber.v("New style loaded with JSON: %s", maplibreMap.style!!.json)
        val adapter = LayerListAdapter(this, layerList)
        val listView = findViewById<ListView>(R.id.listView)
        listView.adapter = adapter
        listView.onItemClickListener =
            AdapterView.OnItemClickListener { parent: AdapterView<*>?, view: View?, position: Int, id: Long ->
                val clickedLayer = adapter.getItem(position)
                toggleLayerVisibility(clickedLayer)
                closeNavigationView()
            }
    }

    private fun toggleLayerVisibility(layer: Layer) {
        val isVisible = layer.visibility.getValue() == Property.VISIBLE
        layer.setProperties(
            PropertyFactory.visibility(
                if (isVisible) Property.NONE else Property.VISIBLE
            )
        )
    }

    private fun closeNavigationView() {
        val drawerLayout = findViewById<DrawerLayout>(R.id.drawer_layout)
        drawerLayout.closeDrawers()
    }

    private fun setupZoomView() {
        val textView = findViewById<TextView>(R.id.textZoom)
        maplibreMap.addOnCameraMoveListener(
            OnCameraMoveListener {
                textView.text = String.format(
                    this@DebugModeActivity.getString(
                        R.string.debug_zoom
                    ),
                    maplibreMap.cameraPosition.zoom
                )
            }.also { cameraMoveListener = it }
        )
    }

    private fun setupDebugChangeView() {
        val fabDebug = findViewById<FloatingActionButton>(R.id.fabDebug)
        fabDebug.setOnClickListener { view: View? ->
            if (this::maplibreMap.isInitialized) {
                maplibreMap.isDebugActive = !maplibreMap.isDebugActive
                Timber.d("Debug FAB: isDebug Active? %s", maplibreMap.isDebugActive)
            }
        }
    }

    private fun setupStyleChangeView() {
        val fabStyles = findViewById<FloatingActionButton>(R.id.fabStyles)
        fabStyles.setOnClickListener { view: View? ->
            if (this::maplibreMap.isInitialized) {
                currentStyleIndex++
                if (currentStyleIndex == STYLES.size) {
                    currentStyleIndex = 0
                }
                maplibreMap.setStyle(Style.Builder().fromUri(STYLES[currentStyleIndex]))
                mapView.setPuckIconState("MorphState")
            }
        }
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        val itemId = item.itemId
        if (itemId == R.id.menu_action_toggle_report_fps) {
            isReportFps = !isReportFps
            fpsView!!.visibility = if (isReportFps) View.VISIBLE else View.GONE
            maplibreMap.setOnFpsChangedListener(if (isReportFps) this else null)
        } else if (itemId == R.id.menu_action_limit_to_30_fps) {
            mapView.setMaximumFps(30)
        } else if (itemId == R.id.menu_action_limit_to_60_fps) {
            mapView.setMaximumFps(60)
        } else if (itemId == R.id.menu_action_toggle_continuous_rendering) {
            isContinuousRendering = !isContinuousRendering
            if (isContinuousRendering) {
                mapView.setRenderingRefreshMode(MapRenderer.RenderingRefreshMode.CONTINUOUS)
            } else {
                mapView.setRenderingRefreshMode(MapRenderer.RenderingRefreshMode.WHEN_DIRTY)
            }
        }
        return actionBarDrawerToggle!!.onOptionsItemSelected(item) || super.onOptionsItemSelected(
            item
        )
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.menu_debug, menu)
        return true
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

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        mapView.onSaveInstanceState(outState)
    }

    override fun onDestroy() {
        super.onDestroy()
        if (this::maplibreMap.isInitialized) {
            maplibreMap.removeOnCameraMoveListener(cameraMoveListener!!)
        }
        mapView.onDestroy()
    }

    override fun onLowMemory() {
        super.onLowMemory()
        mapView.onLowMemory()
    }

    private class LayerListAdapter(context: Context?, layers: List<Layer>) :
        BaseAdapter() {
        private val layoutInflater: LayoutInflater
        private val layers: List<Layer>
        override fun getCount(): Int {
            return layers.size
        }

        override fun getItem(position: Int): Layer {
            return layers[position]
        }

        override fun getItemId(position: Int): Long {
            return position.toLong()
        }

        override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
            val layer = layers[position]
            var view = convertView
            if (view == null) {
                view = layoutInflater.inflate(android.R.layout.simple_list_item_2, parent, false)
                val holder = ViewHolder(
                    view.findViewById(android.R.id.text1),
                    view.findViewById(android.R.id.text2)
                )
                view.tag = holder
            }
            val holder = view!!.tag as ViewHolder
            holder.text.text = layer.javaClass.simpleName
            holder.subText.text = layer.id
            return view
        }

        private class ViewHolder(val text: TextView, val subText: TextView)

        init {
            layoutInflater = LayoutInflater.from(context)
            this.layers = layers
        }
    }

    companion object {
        private val STYLES = arrayOf(
            TestStyles.AMERICANA,
            TestStyles.getPredefinedStyleWithFallback("Streets"),
            TestStyles.getPredefinedStyleWithFallback("Outdoor"),
            TestStyles.getPredefinedStyleWithFallback("Bright"),
            TestStyles.getPredefinedStyleWithFallback("Pastel"),
            TestStyles.getPredefinedStyleWithFallback("Satellite Hybrid"),
            TestStyles.getPredefinedStyleWithFallback("Satellite Hybrid")
        )
    }
}
