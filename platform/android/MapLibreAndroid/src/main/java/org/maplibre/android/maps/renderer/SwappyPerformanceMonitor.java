package org.maplibre.android.maps.renderer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import org.maplibre.android.log.Logger;
import androidx.annotation.Keep;

/**
 * SwappyPerformanceMonitor provides high-level utilities for monitoring
 * and analyzing frame pacing performance using the Android Frame Pacing library.
 *
 * This class makes it easy to:
 * - Monitor SurfaceFlinger delays
 * - Detect frame drops
 * - Analyze rendering performance
 * - Apply performance optimizations
 */
@Keep
public class SwappyPerformanceMonitor {

    private static final String TAG = "Swappy";

    // Performance thresholds
    private static final double GOOD_ON_TIME_PERCENTAGE = 95.0;
    private static final double ACCEPTABLE_ON_TIME_PERCENTAGE = 90.0;
    private static final double HIGH_COMPOSITOR_DELAY_THRESHOLD = 10.0;

    private static int frameCount = 0;
    private static long lastStatsLogTime = 0;
    private static final long STATS_LOG_INTERVAL_MS = 10000; // Log every 10 seconds

    // Emergency fixes tracking
    private static boolean emergencyFixesApplied = false;

    // Note: Frame timing collection has been moved to native C++
    // Use SwappyRenderer.enableNativeFrameTimingCollection() instead

    /**
     * Call this at the beginning of each frame to record frame start for statistics.
     *
     * @param display EGL display handle
     * @param surface EGL surface handle
     */
    public static void recordFrameStart(long display, long surface) {
        SwappyRenderer.recordFrameStart(display, surface);

        frameCount++;

        // Periodically log statistics
        long currentTime = System.currentTimeMillis();
        if (currentTime - lastStatsLogTime >= STATS_LOG_INTERVAL_MS) {
            logPerformanceAnalysis();
            if(SwappyRenderer.isNativeFrameTimingEnabled()) {
                FrameTimingStats timingStats = getNativeFrameTimingStats();
                if (timingStats != null) {
                    Logger.i(TAG, "Native Frame Timing Stats: " + timingStats.toCompactString());
                }
            }
            lastStatsLogTime = currentTime;
        }
    }

    /**
     * Get current frame performance statistics.
     *
     * @return SwappyFrameStats object or null if unavailable
     */
    @Nullable
    public static SwappyFrameStats getCurrentStats() {
        return SwappyRenderer.getStats();
    }

    /**
     * Analyze current performance and log recommendations.
     */
    public static void logPerformanceAnalysis() {
        SwappyFrameStats stats = getCurrentStats();
        if (stats == null) {
            Logger.w(TAG, "Swappy statistics not available");
            return;
        }

        Logger.i(TAG, "=== Frame Performance Analysis ===");
        Logger.i(TAG, stats.toString());

        // Analyze performance and provide recommendations
        analyzeAndRecommend(stats);

        Logger.i(TAG, "=== End Analysis ===");
    }

    /**
     * Analyze statistics and provide performance recommendations.
     */
    private static void analyzeAndRecommend(@NonNull SwappyFrameStats stats) {
        double onTimePercentage = stats.getOnTimeFramePercentage();
        double compositorDelayPercentage = stats.getCompositorDelayPercentage();
        double consistencyScore = stats.getFrameConsistencyScore();

        // Overall performance assessment
        if (onTimePercentage >= GOOD_ON_TIME_PERCENTAGE) {
            Logger.i(TAG, "✅ Excellent frame delivery performance");
        } else if (onTimePercentage >= ACCEPTABLE_ON_TIME_PERCENTAGE) {
            Logger.w(TAG, "Acceptable frame delivery, room for improvement");
        } else {
            Logger.e(TAG, "Poor frame delivery performance - optimization needed");
        }

        // SurfaceFlinger/Compositor analysis
        if (compositorDelayPercentage > HIGH_COMPOSITOR_DELAY_THRESHOLD) {
            Logger.w(TAG, "High SurfaceFlinger delays detected (" + String.format("%.1f", compositorDelayPercentage) + "%)");
            Logger.i(TAG, "Consider: Reduce rendering complexity or enable buffer stuffing fix");
        }

        // Frame consistency analysis
        if (consistencyScore < 80.0) {
            Logger.w(TAG, "Inconsistent frame timing detected");
            Logger.i(TAG, "Consider: Reset frame pacing after scene changes");
        }

        // Specific recommendations
        long totalLateFrames = stats.getTotalLateFrames();
        long totalIdleFrames = stats.getTotalIdleFrames();

        if (totalLateFrames > totalIdleFrames * 2) {
            Logger.i(TAG, "Recommendation: Rendering bottleneck - optimize GPU/CPU work");
        } else if (totalIdleFrames > totalLateFrames * 2) {
            Logger.i(TAG, "Recommendation: Compositor bottleneck - try buffer stuffing fix");
        }
    }

    /**
     * Apply automatic performance optimizations based on current statistics.
     * This method analyzes performance and applies fixes automatically.
     *
     * @return true if optimizations were applied, false otherwise
     */
    public static boolean applyAutoOptimizations() {
        SwappyFrameStats stats = getCurrentStats();
        if (stats == null || stats.getTotalFrames() < 100) {
            return false; // Need sufficient data for analysis
        }

        boolean optimizationsApplied = false;

        // Apply buffer stuffing fix if high compositor delays
        if (stats.getCompositorDelayPercentage() > HIGH_COMPOSITOR_DELAY_THRESHOLD) {
            Logger.i(TAG, "🔧 Applying buffer stuffing fix for high compositor delays");
            SwappyRenderer.setBufferStuffingFixWait(2);
            optimizationsApplied = true;
        }

        // Reset frame pacing if consistency is poor
        if (stats.getFrameConsistencyScore() < 70.0) {
            Logger.i(TAG, "🔧 Resetting frame pacing due to poor consistency");
            SwappyRenderer.resetFramePacing();
            optimizationsApplied = true;
        }

        return optimizationsApplied;
    }

    /**
     * Check if frame performance is within acceptable limits.
     *
     * @return true if performance is acceptable, false if optimization needed
     */
    public static boolean isPerformanceAcceptable() {
        SwappyFrameStats stats = getCurrentStats();
        if (stats == null) {
            return true; // Assume acceptable if no data
        }

        return stats.getOnTimeFramePercentage() >= ACCEPTABLE_ON_TIME_PERCENTAGE;
    }

    /**
     * Get a human-readable performance summary.
     *
     * @return Performance summary string
     */
    @NonNull
    public static String getPerformanceSummary() {
        SwappyFrameStats stats = getCurrentStats();
        if (stats == null) {
            return "Swappy statistics not available";
        }

        return String.format(
            "Frame Performance: %.1f%% on-time, %.1f%% compositor delays, %.1f%% consistency",
            stats.getOnTimeFramePercentage(),
            stats.getCompositorDelayPercentage(),
            stats.getFrameConsistencyScore()
        );
    }

    /**
     * Clear all statistics and reset monitoring.
     */
    public static void reset() {
        SwappyRenderer.clearStats();
        frameCount = 0;
        lastStatsLogTime = 0;
        emergencyFixesApplied = false;

        // Clear native frame timing samples
        SwappyRenderer.clearNativeFrameTimingStats();

        Logger.i(TAG, "Performance monitoring reset");
    }

    // === NATIVE FRAME TIMING ===
    /**
     * Get current frame timing statistics from native C++ implementation.
     * Returns statistical analysis of CPU, GPU, and total pipeline timing.
     *
     * @return FrameTimingStats containing timing analysis, or null if no samples collected
     */
    @Nullable
    public static FrameTimingStats getNativeFrameTimingStats() {
        long[] nativeStats = SwappyRenderer.getNativeFrameTimingStats();
        if (nativeStats == null) {
            return null;
        }

        try {
            return new FrameTimingStats(nativeStats);
        } catch (IllegalArgumentException e) {
            Logger.w(TAG, "Failed to parse native frame timing stats: " + e.getMessage());
            return null;
        }
    }

    /**
     * Enable or disable native frame timing collection.
     * This replaces the old Java-based timing collection with native C++ implementation.
     *
     * @param enabled Whether to enable native frame timing collection
     */
    public static void enableNativeFrameTimingCollection(boolean enabled) {
        SwappyRenderer.enableNativeFrameTimingCollection(enabled);
        if (enabled) {
            Logger.i(TAG, "Native frame timing collection enabled");
        } else {
            Logger.i(TAG, "Native frame timing collection disabled");
        }
    }

    /**
     * Clear native frame timing statistics and reset collection.
     */
    public static void clearNativeFrameTimingStats() {
        SwappyRenderer.clearNativeFrameTimingStats();
        Logger.i(TAG, "Native frame timing statistics cleared");
    }

    /**
     * Emergency diagnostics for severe performance issues.
     * Logs detailed breakdown of frame timing issues to help identify root causes.
     */
    public static void emergencyDiagnostics() {
        SwappyFrameStats stats = SwappyRenderer.getStats();
        if (stats == null) {
            Logger.e(TAG, "=== EMERGENCY DIAGNOSTICS ===");
            Logger.e(TAG, "ERROR: No Swappy statistics available!");
            Logger.e(TAG, "Check if Swappy is initialized and stats are enabled");
            Logger.e(TAG, "=== END DIAGNOSTICS ===");
            return;
        }

        Logger.e(TAG, "=== EMERGENCY DIAGNOSTICS ===");
        Logger.e(TAG, "Total Frames: " + stats.getTotalFrames());
        Logger.e(TAG, "Overall Performance: " + stats.toString());

        // Detailed idle frame breakdown (SurfaceFlinger delays)
        Logger.e(TAG, "--- Compositor Queue Delays (idleFrames) ---");
        long[] idleFrames = stats.getIdleFrames();
        long totalIdle = 0;
        for (int i = 0; i < idleFrames.length; i++) {
            if (idleFrames[i] > 0) {
                double percentage = (double)idleFrames[i] / stats.getTotalFrames() * 100.0;
                Logger.e(TAG, String.format("Frames idle for %d refresh periods: %d (%.1f%%)",
                         i, idleFrames[i], percentage));
                totalIdle += idleFrames[i];
            }
        }
        Logger.e(TAG, String.format("Total idle frames: %d (%.1f%% of all frames)",
                 totalIdle, (double)totalIdle / stats.getTotalFrames() * 100.0));

        // Detailed late frame breakdown (missed deadlines)
        Logger.e(TAG, "--- Missed Presentation Deadlines (lateFrames) ---");
        long[] lateFrames = stats.getLateFrames();
        long totalLate = 0;
        for (int i = 0; i < lateFrames.length; i++) {
            if (lateFrames[i] > 0) {
                double percentage = (double)lateFrames[i] / stats.getTotalFrames() * 100.0;
                Logger.e(TAG, String.format("Frames %d refresh periods late: %d (%.1f%%)",
                         i, lateFrames[i], percentage));
                totalLate += lateFrames[i];
            }
        }
        Logger.e(TAG, String.format("Total late frames: %d (%.1f%% of all frames)",
                 totalLate, (double)totalLate / stats.getTotalFrames() * 100.0));

        // Frame timing consistency analysis
        Logger.e(TAG, "--- Frame Timing Consistency (offsetFromPreviousFrame) ---");
        long[] offsetFrames = stats.getOffsetFromPreviousFrame();
        for (int i = 0; i < offsetFrames.length; i++) {
            if (offsetFrames[i] > 0) {
                double percentage = (double)offsetFrames[i] / stats.getTotalFrames() * 100.0;
                Logger.e(TAG, String.format("Frames with %d refresh periods since previous: %d (%.1f%%)",
                         i, offsetFrames[i], percentage));
            }
        }

        // End-to-end latency analysis
        Logger.e(TAG, "--- End-to-End Latency (latencyFrames) ---");
        long[] latencyFrames = stats.getLatencyFrames();
        for (int i = 0; i < latencyFrames.length; i++) {
            if (latencyFrames[i] > 0) {
                double percentage = (double)latencyFrames[i] / stats.getTotalFrames() * 100.0;
                Logger.e(TAG, String.format("Frames with %d refresh periods total latency: %d (%.1f%%)",
                         i, latencyFrames[i], percentage));
            }
        }

        // === DIAGNOSTIC RECOMMENDATIONS ===
        Logger.e(TAG, "--- DIAGNOSTIC RECOMMENDATIONS ---");

        if (totalIdle > stats.getTotalFrames() * 0.8) {
            Logger.e(TAG, "🚨 SEVERE: >80% frames waiting in compositor queue");
            Logger.e(TAG, "💡 SOLUTION: Apply buffer stuffing fix immediately");
            Logger.e(TAG, "   SwappyRenderer.setBufferStuffingFixWait(3);");
        }

        if (totalLate > stats.getTotalFrames() * 0.9) {
            Logger.e(TAG, "🚨 SEVERE: >90% frames missing deadlines");
            Logger.e(TAG, "💡 SOLUTION: Reduce target frame rate");
            Logger.e(TAG, "   SwappyRenderer.setTargetFrameRate(30);");
        }

        if (totalIdle > totalLate) {
            Logger.e(TAG, "🔍 ANALYSIS: Compositor bottleneck (not rendering bottleneck)");
            Logger.e(TAG, "💡 PRIMARY: Buffer stuffing fix should resolve this");
        } else {
            Logger.e(TAG, "🔍 ANALYSIS: Rendering bottleneck (CPU/GPU performance)");
            Logger.e(TAG, "💡 PRIMARY: Optimize rendering or reduce frame rate");
        }

        // Check for specific patterns
        if (idleFrames.length > 3 && idleFrames[3] > stats.getTotalFrames() * 0.3) {
            Logger.e(TAG, "🚨 PATTERN: Frames waiting 3+ refresh periods");
            Logger.e(TAG, "💡 This is classic buffer stuffing - apply fix immediately");
        }

        Logger.e(TAG, "=== END EMERGENCY DIAGNOSTICS ===");
    }

    /**
     * Apply emergency fixes for severe performance issues.
     * Call this when diagnostics show critical problems.
     */
    public static void applyEmergencyFixes() {
        if (emergencyFixesApplied) {
            Logger.w(TAG, "Emergency fixes already applied, skipping to avoid repeated application");
            return;
        }

        SwappyFrameStats stats = SwappyRenderer.getStats();
        if (stats == null) {
            Logger.e(TAG, "Cannot apply emergency fixes - no stats available");
            return;
        }

        Logger.w(TAG, "=== APPLYING EMERGENCY FIXES ===");

        double compositorDelay = stats.getCompositorDelayPercentage();

        // Emergency fix for severe compositor delays (like your 100%)
        if (compositorDelay > 80.0) {
            Logger.w(TAG, "Applying aggressive buffer stuffing fix for severe delays");
            SwappyRenderer.setBufferStuffingFixWait(3);
        }

        // Reset frame pacing to clear bad timing history
        Logger.w(TAG, "Resetting frame pacing to clear problematic timing history");
        SwappyRenderer.resetFramePacing();

        // Clear stats to get fresh data after fixes
        SwappyRenderer.clearStats();

        emergencyFixesApplied = true;
        Logger.w(TAG, "=== EMERGENCY FIXES APPLIED - MONITOR FOR IMPROVEMENT ===");
    }
}
