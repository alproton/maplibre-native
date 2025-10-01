package org.maplibre.android.location;

import android.content.Context;
import android.os.Handler;
import android.os.SystemClock;
import androidx.annotation.NonNull;

import org.maplibre.android.geometry.LatLng;
import org.maplibre.android.location.modes.CameraMode;
import org.maplibre.android.maps.MapView;

import java.util.Timer;
import java.util.TimerTask;

import static org.maplibre.android.location.Utils.normalize;
import static org.maplibre.android.location.Utils.shortestRotation;

final class LocationAnimatorCustomPuck {

  private Timer puckUpdateTimer;
  private Handler mainHandler;
  private Runnable mainRunnable;
  private StampedLatLon currentPuckLocation;
  private StampedLatLon previousPuckLocation;
  private StampedLatLon targetPuckLocation;
  private StampedLatLon lastValidLocation = null;
  private StampedLatLon sampledLocation = null;
  private long puckInterpolationStartTime = 0;
  private MapView mapview = null;
  private LocationCameraController cameraController = null;
  private LocationAnimatorCustomPuckOptions puckAnimationOptions = null;
  private boolean styleChanged = true;

  class StampedLatLon {
    public double lat;
    public double lon;
    public double bearing;
    public long time;

    public StampedLatLon() {
      lat = 0.0;
      lon = 0.0;
      bearing = 0.0;
      time = 0;
    }

    public StampedLatLon(double lat, double lon, double bearing, long time) {
      this.lat = lat;
      this.lon = lon;
      this.bearing = bearing;
      this.time = time;
      if (Double.isNaN(lat) || Double.isNaN(lon) || Double.isNaN(bearing)) {
        throw new RuntimeException("Unexpected puck location: lat=" + lat + ", lon=" + lon + ", bearing=" + bearing);
      }
    }

    public StampedLatLon(StampedLatLon other) {
      this.lat = other.lat;
      this.lon = other.lon;
      this.bearing = other.bearing;
      this.time = other.time;
    }

    public void copyFrom(StampedLatLon other) {
      this.lat = other.lat;
      this.lon = other.lon;
      this.bearing = other.bearing;
      this.time = other.time;
    }

    boolean equals(StampedLatLon other) {
      return other.lat == lat && other.lon == lon && other.bearing == bearing && other.time == time;
    }
  }

  private void lerp(StampedLatLon a, StampedLatLon b, double t) {
    double bearingA = (double)(normalize((float)(a.bearing)));
    double bearingB = (double)(shortestRotation((float)(b.bearing), (float)(bearingA)));
    sampledLocation.lat = a.lat * (1.0 - t) + b.lat * t;
    sampledLocation.lon = a.lon * (1.0 - t) + b.lon * t;
    sampledLocation.bearing = bearingA * (1.0 - t) + bearingB * t;
    sampledLocation.time = (long)((double)(a.time) * (1.0 - t) + (double)(b.time) * t);
  }

  void updateLocation(double lat, double lon, double bearing, long time) {
    currentPuckLocation = new StampedLatLon(lat, lon, bearing, time);
    lastValidLocation = new StampedLatLon(currentPuckLocation);
  }

  public void cameraChange() {
   if (mapview == null
       || mapview.getMapLibreMap() == null
       || mapview.getMapLibreMap().getStyle() == null
       || lastValidLocation == null
       || cameraController == null
       || puckAnimationOptions == null) {
      return;
    }
    if (cameraController.isLocationTracking()) {
      cameraController.setLatLng(new LatLng(lastValidLocation.lat, lastValidLocation.lon));
    }
    if (cameraController.isLocationBearingTracking()) {
      float bearing = (float)(lastValidLocation.bearing);
      if (cameraController.getCameraMode() == CameraMode.TRACKING_GPS_NORTH) {
        bearing = 0.0f;
      }
      cameraController.setBearing(bearing);
    }
    boolean tracking = cameraController.isLocationTracking() && !cameraController.isTransitioning();
    mapview.setCustomPuckState(
      lastValidLocation.lat,
      lastValidLocation.lon,
      lastValidLocation.bearing,
      puckAnimationOptions.iconScale,
      tracking);
  }

  public void styleChange() {
    styleChanged = true;
  }

  LocationAnimatorCustomPuck(@NonNull Context context,
                             @NonNull MapView mapView,
                             @NonNull LocationLayerRenderer locationLayerRenderer,
                             @NonNull LocationCameraController locationCameraController,
                             @NonNull LocationAnimatorCustomPuckOptions customPuckAnimationOptions) {
    mapview = mapView;
    cameraController = locationCameraController;
    puckAnimationOptions = customPuckAnimationOptions;

    // Initialize sampledLocation with invalid zero data
    sampledLocation = new StampedLatLon();
    LatLng sampledLatLon = new LatLng();

    // Mbgl-Layer interactions must happen on the UI thread. i.e.
    // locationLayerRenderer methods cannot be called from the TimerTask thread
    // We use get a mainLooper handler to do Mbgl-Layer interactions
    mainHandler = new Handler(context.getMainLooper());

    // mainRunnable is what mainHandler posts
    mainRunnable = new Runnable() {
      @Override
      public void run() {
        // Make sure the style is loaded before using locationLayerRenderer
        if (mapView != null
            && mapView.getMapLibreMap() != null
            && mapView.getMapLibreMap().getStyle() != null
            && mapView.getMapLibreMap().getStyle().isFullyLoaded()) {
          locationLayerRenderer.setLatLng(sampledLatLon);
          locationLayerRenderer.setGpsBearing((float)(sampledLocation.bearing));
          if (styleChanged) {
            locationLayerRenderer.hide();
            styleChanged = false;
          }
        }

        if (locationCameraController.isLocationTracking()) {
          locationCameraController.setLatLng(sampledLatLon);
        }
        if (locationCameraController.isLocationBearingTracking()) {
          float bearing = (float)(sampledLocation.bearing);
          if (locationCameraController.getCameraMode() == CameraMode.TRACKING_GPS_NORTH) {
            bearing = 0.0f;
          }
          locationCameraController.setBearing(bearing);
        }

        boolean tracking = locationCameraController.isLocationTracking() && !locationCameraController.isTransitioning();
        if (mapView != null) {
            mapView.setCustomPuckState(
            sampledLocation.lat,
            sampledLocation.lon,
            sampledLocation.bearing,
            customPuckAnimationOptions.iconScale,
            tracking);
          }
        }
    };

    puckUpdateTimer = new Timer();
    puckUpdateTimer.schedule(new TimerTask() {
      @Override
      public void run() {
        if (currentPuckLocation == null) {
          return;
        }
        if (previousPuckLocation == null) {
          previousPuckLocation = new StampedLatLon(currentPuckLocation);
          puckInterpolationStartTime = SystemClock.elapsedRealtime();
        }
        if (targetPuckLocation == null) {
          targetPuckLocation = new StampedLatLon(currentPuckLocation);
          puckInterpolationStartTime = SystemClock.elapsedRealtime();
        }

        long elapsed = SystemClock.elapsedRealtime() - puckInterpolationStartTime;
        if (elapsed > customPuckAnimationOptions.lagMS) {
          // Stale data. Wait for next location update
          currentPuckLocation = null;
          previousPuckLocation = null;
          targetPuckLocation = null;
          return;
        }
        double t = (double)(elapsed) / (double)(customPuckAnimationOptions.lagMS);
        lerp(previousPuckLocation, targetPuckLocation, t);
        sampledLatLon.setLatitude(sampledLocation.lat);
        sampledLatLon.setLongitude(sampledLocation.lon);

        if (!targetPuckLocation.equals(currentPuckLocation)) {
          puckInterpolationStartTime = SystemClock.elapsedRealtime();
          previousPuckLocation.copyFrom(sampledLocation);
          targetPuckLocation.copyFrom(currentPuckLocation);
        }

        // On Android post also acts as a memory barrier so no need for volatile variables
        mainHandler.post(mainRunnable);
      }
    }, 0, customPuckAnimationOptions.animationIntervalMS);
  }

  public void onDestroy() {
    puckUpdateTimer.cancel();
    puckUpdateTimer.purge();
    puckUpdateTimer = null;

    mainHandler.removeCallbacksAndMessages(null);
    mainHandler = null;
  }
}
