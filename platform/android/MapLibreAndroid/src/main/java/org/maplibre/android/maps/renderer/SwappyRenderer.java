package org.maplibre.android.maps.renderer;

import android.app.Activity;
import android.view.Surface;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * SwappyRenderer provides Android Frame Pacing (Swappy) integration for MapLibre.
 * This class wraps the native Swappy API to provide smooth frame pacing and reduce jank.
 *
 * Based on the Android Frame Pacing documentation:
 * https://developer.android.com/games/sdk/frame-pacing/opengl/add-functions
 */
@Keep
public class SwappyRenderer {

    private static boolean sInitialized = false;
    private static boolean sEnabled = false;

    /**
     * Initialize Swappy Frame Pacing.
     * Should be called early in the application lifecycle, typically in onCreate().
     *
     * @param activity The current activity
     * @return true if initialization was successful, false otherwise
     */
    public static boolean initialize(@NonNull Activity activity) {
        if (sInitialized) {
            return sEnabled;
        }

        try {
            sEnabled = nativeInitialize(activity);
            sInitialized = true;
            return sEnabled;
        } catch (UnsatisfiedLinkError e) {
            // Native library not loaded or Swappy not available
            sInitialized = true;
            sEnabled = false;
            return false;
        }
    }

    /**
     * Destroy Swappy Frame Pacing.
     * Should be called when the application is exiting.
     */
    public static void destroy() {
        if (!sInitialized) {
            return;
        }

        try {
            nativeDestroy();
        } catch (UnsatisfiedLinkError e) {
            // Ignore - native library might not be loaded
        }

        sInitialized = false;
        sEnabled = false;
    }

    /**
     * Check if Swappy Frame Pacing is enabled and available.
     *
     * @return true if Swappy is enabled, false otherwise
     */
    public static boolean isEnabled() {
        if (!sInitialized) {
            return false;
        }

        try {
            return nativeIsEnabled();
        } catch (UnsatisfiedLinkError e) {
            return false;
        }
    }

    /**
     * Set the target frame rate for frame pacing.
     *
     * @param targetFps Target frame rate (e.g., 60, 30, 20)
     */
    public static void setTargetFrameRate(int targetFps) {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeSetTargetFrameRate(targetFps);
        } catch (UnsatisfiedLinkError e) {
            // Ignore - fallback to standard frame pacing
        }
    }

    /**
     * Perform frame swap using Swappy.
     * This should replace eglSwapBuffers() calls when Swappy is available.
     *
     * @param display EGL display handle (as long)
     * @param surface EGL surface handle (as long)
     * @return true if swap was successful, false otherwise
     */
    public static boolean swap(long display, long surface) {
        if (!sInitialized) {
            return false;
        }

        try {
            return nativeSwap(display, surface);
        } catch (UnsatisfiedLinkError e) {
            return false;
        }
    }

    /**
     * Set the native window for Swappy.
     * Should be called when the display surface changes.
     *
     * @param surface The surface to use
     */
    public static void setWindow(@Nullable Surface surface) {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeSetWindow(surface);
        } catch (UnsatisfiedLinkError e) {
            // Ignore - fallback to standard rendering
        }
    }

    // === METRICS AND STATISTICS ===

    /**
     * Enable or disable statistics collection.
     * When enabled, Swappy collects detailed frame timing metrics.
     *
     * @param enabled Whether to enable statistics collection
     */
    public static void enableStats(boolean enabled) {
        if (!sInitialized) {
            return;
        }

        try {
            nativeEnableStats(enabled);
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    /**
     * Record the start of a frame for statistics collection.
     * Should be called at the beginning of each frame when stats are enabled.
     *
     * @param display EGL display handle (as long)
     * @param surface EGL surface handle (as long)
     */
    public static void recordFrameStart(long display, long surface) {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeRecordFrameStart(display, surface);
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    /**
     * Get collected frame statistics.
     * Returns a SwappyFrameStats object with detailed metrics.
     *
     * @return SwappyFrameStats object or null if stats unavailable
     */
    @Nullable
    public static SwappyFrameStats getStats() {
        if (!sInitialized || !sEnabled) {
            return null;
        }

        try {
            long[] rawStats = nativeGetStats();
            if (rawStats != null && rawStats.length == 25) {
                return new SwappyFrameStats(rawStats);
            }
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
        return null;
    }

    /**
     * Clear all collected frame statistics.
     * Resets all counters to zero.
     */
    public static void clearStats() {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeClearStats();
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    /**
     * Log comprehensive frame statistics to help analyze performance.
     * This logs detailed metrics to the Android log for debugging.
     */
    public static void logFrameStats() {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeLogFrameStats();
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    // === ADVANCED CONFIGURATION ===

    /**
     * Set the buffer stuffing fix wait time.
     * This addresses GPU driver bugs where extra frames are "stuffed" into the pipeline.
     *
     * @param nFrames Number of bad frames to wait before applying the fix (0 = disabled)
     */
    public static void setBufferStuffingFixWait(int nFrames) {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeSetBufferStuffingFixWait(nFrames);
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    /**
     * Enable or disable frame pacing entirely.
     * When disabled, frames are presented as soon as possible.
     *
     * @param enabled Whether to enable frame pacing
     */
    public static void enableFramePacing(boolean enabled) {
        if (!sInitialized) {
            return;
        }

        try {
            nativeEnableFramePacing(enabled);
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    /**
     * Reset the frame pacing mechanism.
     * Clears frame timing history - useful after scene transitions or loading screens.
     */
    public static void resetFramePacing() {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeResetFramePacing();
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    /**
     * Set maximum auto swap interval for adaptive frame timing.
     *
     * @param maxSwapIntervalNs Maximum swap interval in nanoseconds
     */
    public static void setMaxAutoSwapInterval(long maxSwapIntervalNs) {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeSetMaxAutoSwapInterval(maxSwapIntervalNs);
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    /**
     * Enable or disable auto swap interval adjustment.
     *
     * @param enabled Whether to enable automatic swap interval adjustment
     */
    public static void setAutoSwapInterval(boolean enabled) {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeSetAutoSwapInterval(enabled);
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    /**
     * Enable or disable auto pipeline mode.
     * Reduces latency by scheduling CPU and GPU work in the same pipeline stage.
     *
     * @param enabled Whether to enable auto pipeline mode
     */
    public static void setAutoPipelineMode(boolean enabled) {
        if (!sInitialized || !sEnabled) {
            return;
        }

        try {
            nativeSetAutoPipelineMode(enabled);
        } catch (UnsatisfiedLinkError e) {
            // Ignore
        }
    }

    // Native method declarations
    @Keep
    private static native boolean nativeInitialize(@NonNull Activity activity);

    @Keep
    private static native void nativeDestroy();

    @Keep
    private static native boolean nativeIsEnabled();

    @Keep
    private static native void nativeSetTargetFrameRate(int targetFps);

    @Keep
    private static native boolean nativeSwap(long display, long surface);

    @Keep
    private static native void nativeSetWindow(@Nullable Surface surface);

    // === METRICS AND STATISTICS NATIVE METHODS ===

    @Keep
    private static native void nativeEnableStats(boolean enabled);

    @Keep
    private static native void nativeRecordFrameStart(long display, long surface);

    @Keep
    private static native long[] nativeGetStats();

    @Keep
    private static native void nativeClearStats();

    @Keep
    private static native void nativeLogFrameStats();

    // === ADVANCED CONFIGURATION NATIVE METHODS ===

    @Keep
    private static native void nativeSetBufferStuffingFixWait(int nFrames);

    @Keep
    private static native void nativeEnableFramePacing(boolean enabled);

    @Keep
    private static native void nativeResetFramePacing();

    @Keep
    private static native void nativeSetMaxAutoSwapInterval(long maxSwapIntervalNs);

    @Keep
    private static native void nativeSetAutoSwapInterval(boolean enabled);

    @Keep
    private static native void nativeSetAutoPipelineMode(boolean enabled);
}
