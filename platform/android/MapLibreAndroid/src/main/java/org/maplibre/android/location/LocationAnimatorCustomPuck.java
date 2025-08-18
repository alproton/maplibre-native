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
  private StampedLatLon currentPuckLocation;
  private StampedLatLon previousPuckLocation;
  private StampedLatLon targetPuckLocation;
  private StampedLatLon lastValidLocation = null;
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

    boolean equals(StampedLatLon other) {
      return other.lat == lat && other.lon == lon && other.bearing == bearing && other.time == time;
    }
  }

  private StampedLatLon lerp(StampedLatLon a, StampedLatLon b, double t) {
    double bearingA = (double)(normalize((float)(a.bearing)));
    double bearingB = (double)(shortestRotation((float)(b.bearing), (float)(bearingA)));
    return new StampedLatLon(a.lat * (1.0 - t) + b.lat * t,
                             a.lon * (1.0 - t) + b.lon * t,
                             bearingA * (1.0 - t) + bearingB * t,
                             (long)((double)(a.time) * (1.0 - t) + (double)(b.time) * t));
  }

  void updateLocation(double lat, double lon, double bearing, long time) {
    currentPuckLocation = new StampedLatLon(lat, lon, bearing, time);
    lastValidLocation = new StampedLatLon(currentPuckLocation);
  }

  public void cameraModeChange() {
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
        StampedLatLon location = lerp(previousPuckLocation, targetPuckLocation, t);

        if (!targetPuckLocation.equals(currentPuckLocation)) {
          puckInterpolationStartTime = SystemClock.elapsedRealtime();
          previousPuckLocation = new StampedLatLon(location);
          targetPuckLocation = new StampedLatLon(currentPuckLocation);
        }

        // Get a handler that can be used to post to the main thread
        Handler mainHandler = new Handler(context.getMainLooper());

        Runnable myRunnable = new Runnable() {
          @Override
          public void run() {
            // Make sure the style is loaded before using locationLayerRenderer
            if (mapView != null
                && mapView.getMapLibreMap() != null
                && mapView.getMapLibreMap().getStyle() != null
                && mapView.getMapLibreMap().getStyle().isFullyLoaded()) {
              locationLayerRenderer.setLatLng(new LatLng(location.lat, location.lon));
              locationLayerRenderer.setGpsBearing((float)(location.bearing));
              if (styleChanged) {
                locationLayerRenderer.hide();
                styleChanged = false;
              }
            }

            if (locationCameraController.isLocationTracking()) {
              locationCameraController.setLatLng(new LatLng(location.lat, location.lon));
            }
            if (locationCameraController.isLocationBearingTracking()) {
              float bearing = (float)(location.bearing);
              if (locationCameraController.getCameraMode() == CameraMode.TRACKING_GPS_NORTH) {
                bearing = 0.0f;
              }
              locationCameraController.setBearing(bearing);
            }

            boolean tracking = locationCameraController.isLocationTracking() && !locationCameraController.isTransitioning();
            if (mapView != null) {
                mapView.setCustomPuckState(
                location.lat,
                location.lon,
                location.bearing,
                customPuckAnimationOptions.iconScale,
                tracking);
              }
            }
        };
        mainHandler.post(myRunnable);
      }
    }, 0, customPuckAnimationOptions.animationIntervalMS);
  }

  public void onDestroy() {
    puckUpdateTimer.cancel();
    puckUpdateTimer.purge();
    puckUpdateTimer = null;
  }
}
