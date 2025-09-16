package org.maplibre.android.maps.renderer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import org.maplibre.android.log.Logger;

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
public class SwappyPerformanceMonitor {

    private static final String TAG = "SwappyMonitor";

    // Performance thresholds
    private static final double GOOD_ON_TIME_PERCENTAGE = 95.0;
    private static final double ACCEPTABLE_ON_TIME_PERCENTAGE = 90.0;
    private static final double HIGH_COMPOSITOR_DELAY_THRESHOLD = 10.0;

    private static int frameCount = 0;
    private static long lastStatsLogTime = 0;
    private static final long STATS_LOG_INTERVAL_MS = 10000; // Log every 10 seconds

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
            Logger.w(TAG, "⚠️ Acceptable frame delivery, room for improvement");
        } else {
            Logger.e(TAG, "❌ Poor frame delivery performance - optimization needed");
        }

        // SurfaceFlinger/Compositor analysis
        if (compositorDelayPercentage > HIGH_COMPOSITOR_DELAY_THRESHOLD) {
            Logger.w(TAG, "🐌 High SurfaceFlinger delays detected (" + String.format("%.1f", compositorDelayPercentage) + "%)");
            Logger.i(TAG, "💡 Consider: Reduce rendering complexity or enable buffer stuffing fix");
        }

        // Frame consistency analysis
        if (consistencyScore < 80.0) {
            Logger.w(TAG, "📊 Inconsistent frame timing detected");
            Logger.i(TAG, "💡 Consider: Reset frame pacing after scene changes");
        }

        // Specific recommendations
        long totalLateFrames = stats.getTotalLateFrames();
        long totalIdleFrames = stats.getTotalIdleFrames();

        if (totalLateFrames > totalIdleFrames * 2) {
            Logger.i(TAG, "💡 Recommendation: Rendering bottleneck - optimize GPU/CPU work");
        } else if (totalIdleFrames > totalLateFrames * 2) {
            Logger.i(TAG, "💡 Recommendation: Compositor bottleneck - try buffer stuffing fix");
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
        Logger.i(TAG, "Performance monitoring reset");
    }
}
