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
  private Runnable updateRunnable;
  private TimerTask puckUpdateTask;
  private StampedLatLon currentPuckLocation = null;
  private StampedLatLon previousPuckLocation = null;
  private StampedLatLon targetPuckLocation = null;
  private StampedLatLon lastValidLocation = null;
  private StampedLatLon lerpResult = null;
  private long puckInterpolationStartTime = 0;
  private MapView mapview = null;
  private LocationCameraController cameraController = null;
  private LocationAnimatorCustomPuckOptions puckAnimationOptions = null;
  private boolean styleChanged = true;

  // Fields to store current location data for the reusable Runnable
  private volatile double currentLat;
  private volatile double currentLon;
  private volatile double currentBearing;

  // Reusable LatLng objects to avoid allocations
  private LatLng reusableLatLng1;
  private LatLng reusableLatLng2;

  class StampedLatLon {
    public double lat;
    public double lon;
    public double bearing;
    public long time;

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

    public boolean equals(StampedLatLon other) {
      return other.lat == lat && other.lon == lon && other.bearing == bearing && other.time == time;
    }

    public void copyFrom(StampedLatLon other) {
      this.lat = other.lat;
      this.lon = other.lon;
      this.bearing = other.bearing;
      this.time = other.time;
    }
  }

  private StampedLatLon lerp(StampedLatLon a, StampedLatLon b, double t) {
    double bearingA = (double)(normalize((float)(a.bearing)));
    double bearingB = (double)(shortestRotation((float)(b.bearing), (float)(bearingA)));

    if (lerpResult == null) {
      lerpResult = new StampedLatLon(0, 0, 0, 0);
    }

    lerpResult.lat = a.lat * (1.0 - t) + b.lat * t;
    lerpResult.lon = a.lon * (1.0 - t) + b.lon * t;
    lerpResult.bearing = bearingA * (1.0 - t) + bearingB * t;
    lerpResult.time = (long)((double)(a.time) * (1.0 - t) + (double)(b.time) * t);

    return lerpResult;
  }

  void updateLocation(double lat, double lon, double bearing, long time) {
    if (currentPuckLocation == null) {
      currentPuckLocation = new StampedLatLon(lat, lon, bearing, time);
    } else {
      currentPuckLocation.lat = lat;
      currentPuckLocation.lon = lon;
      currentPuckLocation.bearing = bearing;
      currentPuckLocation.time = time;
    }

    if (previousPuckLocation == null) {
      previousPuckLocation = new StampedLatLon(currentPuckLocation);
    } else {
      previousPuckLocation.copyFrom(currentPuckLocation);
    }
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
      reusableLatLng1.setLatitude(lastValidLocation.lat);
      reusableLatLng1.setLongitude(lastValidLocation.lon);
      cameraController.setLatLng(reusableLatLng1);
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
    mainHandler = new Handler(context.getMainLooper());

    // Initialize reusable LatLng objects
    reusableLatLng1 = new LatLng(0, 0);
    reusableLatLng2 = new LatLng(0, 0);

    // Create reusable Runnable
    updateRunnable = new Runnable() {
      @Override
      public void run() {
        // Make sure the style is loaded before using locationLayerRenderer
        if (mapView != null
            && mapView.getMapLibreMap() != null
            && mapView.getMapLibreMap().getStyle() != null
            && mapView.getMapLibreMap().getStyle().isFullyLoaded()) {
          reusableLatLng1.setLatitude(currentLat);
          reusableLatLng1.setLongitude(currentLon);
          locationLayerRenderer.setLatLng(reusableLatLng1);
          locationLayerRenderer.setGpsBearing((float)(currentBearing));
          if (styleChanged) {
            locationLayerRenderer.hide();
            styleChanged = false;
          }
        }

        if (locationCameraController.isLocationTracking()) {
          reusableLatLng2.setLatitude(currentLat);
          reusableLatLng2.setLongitude(currentLon);
          locationCameraController.setLatLng(reusableLatLng2);
        }
        if (locationCameraController.isLocationBearingTracking()) {
          float bearing = (float)(currentBearing);
          if (locationCameraController.getCameraMode() == CameraMode.TRACKING_GPS_NORTH) {
            bearing = 0.0f;
          }
          locationCameraController.setBearing(bearing);
        }

        boolean tracking = locationCameraController.isLocationTracking() && !locationCameraController.isTransitioning();
        if (mapView != null) {
            mapView.setCustomPuckState(
            currentLat,
            currentLon,
            currentBearing,
            customPuckAnimationOptions.iconScale,
            tracking);
          }
        }
    };

    // Create reusable TimerTask
    puckUpdateTask = new TimerTask() {
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
        StampedLatLon location = lerp(previousPuckLocation, targetPuckLocation, t);

        if (!targetPuckLocation.equals(currentPuckLocation)) {
          puckInterpolationStartTime = SystemClock.elapsedRealtime();
          previousPuckLocation.copyFrom(location);
          targetPuckLocation.copyFrom(currentPuckLocation);
        }

        // Update the current location data for the reusable Runnable
        currentLat = location.lat;
        currentLon = location.lon;
        currentBearing = location.bearing;

        mainHandler.post(updateRunnable);
      }
    };

    puckUpdateTimer = new Timer();
    puckUpdateTimer.scheduleAtFixedRate(puckUpdateTask, 0, customPuckAnimationOptions.animationIntervalMS);
  }

  public void onDestroy() {
    puckUpdateTimer.cancel();
    puckUpdateTimer.purge();
    puckUpdateTimer = null;
    mainHandler.removeCallbacksAndMessages(null);
    mainHandler = null;
  }
}
