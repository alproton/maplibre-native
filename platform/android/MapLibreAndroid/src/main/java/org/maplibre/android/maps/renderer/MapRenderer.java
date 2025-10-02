package org.maplibre.android.maps.renderer;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import java.lang.ref.WeakReference;

import androidx.annotation.CallSuper;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.maplibre.android.LibraryLoader;
import org.maplibre.android.log.Logger;
import org.maplibre.android.maps.MapLibreMap;
import org.maplibre.android.maps.MapLibreMapOptions;
import org.maplibre.android.maps.renderer.surfaceview.MapLibreSurfaceView;

/**
 * The {@link MapRenderer} encapsulates the render thread.
 * <p>
 * Performs actions on the render thread to manage the resources and
 * render on the one end and acts as a scheduler to request work to
 * be performed on the render thread on the other.
 */
@Keep
public abstract class MapRenderer implements MapRendererScheduler {

  static {
    LibraryLoader.load();
  }

  private static final String TAG = "Mbgl-MapRenderer";

  /**
   * Rendering presentation refresh mode.
   */
  public enum RenderingRefreshMode {
    /**
     * The map is rendered only in response to an event that affects the rendering of the map.
     * This mode is preferred to improve battery life and overall system performance
     */
    WHEN_DIRTY,

    /**
     * The map is repeatedly re-rendered at the refresh rate of the display.
     * This mode is preferred when benchmarking the rendering
     */
    CONTINUOUS,
  }

  // Holds the pointer to the native peer after initialization
  private long nativePtr = 0;
  private double expectedRenderTime = 0;
  private boolean skipWaitingFrames = false;
  private MapLibreMap.OnFpsChangedListener onFpsChangedListener;

  // Swappy initialization in render thread
  private WeakReference<Context> contextRef;
  private boolean shouldEnableSwappy = false;
  private boolean enableSwappyLogs = false;
  private boolean swappyInitialized = false;
  private boolean shouldEnableSwappyFrameMetrics = false;

  public static MapRenderer create(MapLibreMapOptions options, @NonNull Context context, Runnable initCallback) {

    MapRenderer renderer = null;
    String localFontFamily = options.getLocalIdeographFontFamily();
    int threadPriorityOverride = options.getThreadPriorityOverride();

    if (options.getTextureMode()) {
      TextureView textureView = new TextureView(context);
      boolean translucentSurface = options.getTranslucentTextureSurface();
      renderer = MapRendererFactory.newTextureViewMapRenderer(context, textureView, localFontFamily,
              translucentSurface, initCallback, threadPriorityOverride);
    } else {
      boolean renderSurfaceOnTop = options.getRenderSurfaceOnTop();
      boolean useModernEGL = options.getUseModernEGL();
      boolean useSwappy = options.getUseSwappy();
      boolean enableSwappyLogs = options.getEnableSwappyLogs();
      boolean enableSwappyFrameMetrics = options.getUseSwappyFrameMetrics();
      renderer = MapRendererFactory.newSurfaceViewMapRenderer(context, localFontFamily,
              renderSurfaceOnTop, initCallback, threadPriorityOverride, useModernEGL);

      // Configure Swappy settings for render thread initialization
      if (renderer != null) {
        renderer.configureSwappyInitialization(context, useSwappy, enableSwappyLogs, enableSwappyFrameMetrics);
      }
    }

    return renderer;
  }

  public MapRenderer(@NonNull Context context, String localIdeographFontFamily, int threadPriorityOverride) {
    float pixelRatio = context.getResources().getDisplayMetrics().density;

    // Initialize native peer
    nativeInitialize(this, pixelRatio, localIdeographFontFamily, threadPriorityOverride);
  }

  public void enableFrameTimingCollection(boolean enable) {
    SwappyRenderer.enableNativeFrameTimingCollection(enable);
  }

  /**
   * Get current frame timing statistics.
   *
   * @return FrameTimingStats object with timing analysis, or null if no samples collected
   */
  @Nullable
  public FrameTimingStats getFrameTimingStats() {
    return SwappyPerformanceMonitor.getNativeFrameTimingStats();
  }

  /**
   * Configure Swappy settings for render thread initialization.
   * This method stores the context and settings to be used later in the render thread.
   *
   * @param context The context to use for Swappy initialization
   * @param useSwappy Whether Swappy should be enabled
   * @param enableLogs Whether Swappy logging should be enabled
   */
  public void configureSwappyInitialization(@NonNull Context context, boolean useSwappy, boolean enableLogs, boolean enableFrameMetrics) {
    this.contextRef = new WeakReference<Context>(context);
    this.shouldEnableSwappy = useSwappy;
    this.enableSwappyLogs = enableLogs;
    this.swappyInitialized = false;
    this.shouldEnableSwappyFrameMetrics = enableFrameMetrics;
  }

  /**
   * Initialize Swappy in the render thread if needed.contextRef
   * This method is called from onDrawFrame and ensures Swappy is initialized only once.
   */
  private void lazyInitializeSwappy() {
    if (!shouldEnableSwappy || swappyInitialized) {
      return; // Either Swappy not needed or already initialized
    }

    Context context = contextRef != null ? contextRef.get() : null;
    if (context == null) {
      Logger.w(TAG, "Context reference is null, cannot initialize Swappy in render thread");
      swappyInitialized = true; // Mark as initialized to avoid repeated attempts
      return;
    }

    Logger.i(TAG, "Initializing Swappy Frame Pacing in render thread");
    initializeSwappy(context, enableSwappyLogs);
    if(shouldEnableSwappyFrameMetrics) {
      enableFrameTimingCollection(true);
    }
    swappyInitialized = true;
  }

  /**
   * Initialize Swappy Frame Pacing if possible.
   * Attempts to find the Activity context and initialize Swappy.
   * This method is now called from the render thread.
   */
  private static void initializeSwappy(@NonNull Context context, boolean enableLogs) {
    try {
      Activity activity = getActivityFromContext(context);
      if (activity != null) {
        boolean initialized = SwappyRenderer.initialize(activity);
        if (initialized) {
          Logger.i("Swappy", "Swappy Frame Pacing initialized successfully");

          // Set default configuration for optimal performance
          SwappyRenderer.setTargetFrameRate(60);
          SwappyRenderer.setUseAffinity(true);
          SwappyRenderer.enableStats(enableLogs);
          SwappyRenderer.setAutoSwapInterval(false);
          SwappyRenderer.setAutoPipelineMode(false);
          SwappyRenderer.resetFramePacing();
          SwappyRenderer.clearStats();
        } else {
          Logger.w(TAG, "Swappy Frame Pacing initialization failed or not supported on this device");
        }
      } else {
        Logger.w(TAG, "Could not find Activity context for Swappy initialization");
      }
    } catch (Exception e) {
      Logger.w(TAG, "Exception during Swappy initialization: " + e.getMessage());
    }
  }

  /**
   * Extract Activity from Context, handling ContextWrapper cases.
   */
  @Nullable
  private static Activity getActivityFromContext(@NonNull Context context) {
    if (context instanceof Activity) {
      return (Activity) context;
    } else if (context instanceof ContextWrapper) {
      return getActivityFromContext(((ContextWrapper) context).getBaseContext());
    }
    return null;
  }


  public abstract View getView();

  public void onStart() {
    // Implement if needed
  }

  public void onPause() {
    // Implement if needed
  }

  public void onResume() {
    // Implement if needed
  }

  public void onStop() {
    // Implement if needed
  }

  public void onDestroy() {
    // Implement if needed
  }

  public abstract void setRenderingRefreshMode(RenderingRefreshMode mode);

  public abstract RenderingRefreshMode getRenderingRefreshMode();

  public void setOnFpsChangedListener(MapLibreMap.OnFpsChangedListener listener) {
    onFpsChangedListener = listener;
  }

  @CallSuper
  protected void onSurfaceCreated(Surface surface) {
    nativeOnSurfaceCreated(surface);
  }

  @CallSuper
  protected void onSurfaceChanged(int width, int height) {
    nativeOnSurfaceChanged(width, height);
  }

  @CallSuper
  protected void onSurfaceDestroyed() {
    nativeOnSurfaceDestroyed();
  }


  @CallSuper
  protected void onDrawFrame(boolean isWaitingFrame) {
    // Initialize Swappy once in the render thread if needed
    lazyInitializeSwappy();

    if(SwappyRenderer.isEnabled()) {
      View view = getView();
      if(view instanceof MapLibreSurfaceView) {
        ((MapLibreSurfaceView) view).recordFrameStart();
      }
    }

    long startTime = System.nanoTime();
    try {
      nativeRender();
    } catch (java.lang.Error error) {
      Logger.e(TAG, error.getMessage());
    }
    //do the old sleep based frame limiting if swappy is not enabled
    if(!SwappyRenderer.isEnabled()) {
      long renderTime = System.nanoTime() - startTime;
      if (renderTime < expectedRenderTime) {
        try {
          Thread.sleep((long) ((expectedRenderTime - renderTime) / 1E6));
        } catch (InterruptedException ex) {
          Logger.e(TAG, ex.getMessage());
        }
      }
    }
    if (onFpsChangedListener != null) {
      updateFps(isWaitingFrame && skipWaitingFrames);
    }
  }

  public void setSwapBehaviorFlush(boolean flush) {
    nativeSetSwapBehaviorFlush(flush);
  }

  public void setSwapInterval(int interval) {
      SwappyPerformanceMonitor.reset();
      nativeSetSwapInterval(interval);
  }

  /**
   * May be called from any thread.
   * <p>
   * Called from the native peer to schedule work on the GL
   * thread. Explicit override for easier to read jni code.
   *
   * @param runnable the runnable to execute
   * @see MapRendererRunnable
   */
  @CallSuper
  void queueEvent(MapRendererRunnable runnable) {
    this.queueEvent((Runnable) runnable);
  }


  private native void nativeInitialize(MapRenderer self,
                                       float pixelRatio,
                                       String localIdeographFontFamily,
                                       int threadPriorityOverride);

  @CallSuper
  @Override
  protected native void finalize() throws Throwable;

  private native void nativeOnSurfaceCreated(Surface surface);

  private native void nativeOnSurfaceChanged(int width, int height);

  private native void nativeOnSurfaceDestroyed();

  protected native void nativeReset();

  private native void nativeRender();

  private native void nativeSetSwapBehaviorFlush(boolean flush);

  private native void nativeSetSwapInterval(int interval);

  public native void nativeSetCustomPuckState(double lat,
                                              double lon,
                                              double bearing,
                                              float iconScale,
                                              boolean cameraTracking);

  private long timeElapsed;

  private void updateFps(boolean skipFrame) {
    long currentTime = System.nanoTime();
    if (timeElapsed > 0) {
      double fps = 1E9 / ((currentTime - timeElapsed));
      if (!skipFrame) {
        onFpsChangedListener.onFpsChanged(fps);
      }
    }
    timeElapsed = currentTime;
  }

  /**
   * The max frame rate at which this render is rendered,
   * but it can't excess the ability of device hardware.
   *
   * @param maximumFps Can be set to arbitrary integer values.
   */
  public void setMaximumFps(int maximumFps) {
    if (maximumFps <= 0) {
      // Not valid, just return
      return;
    }

    if(SwappyRenderer.isEnabled()) {
      Logger.i("Swappy", "Setting Maximum FPS to: " + maximumFps);
      SwappyPerformanceMonitor.reset();
      SwappyRenderer.setTargetFrameRate(maximumFps);
    } else {
      expectedRenderTime = 1E9 / maximumFps;
    }
  }

  /**
   * Skip frames waiting for a repaint
   *
   * @param skip Can be set to true or false.
   */
  public void setSkipWaitingFrames(boolean skip) {
    skipWaitingFrames = skip;
  }
}
