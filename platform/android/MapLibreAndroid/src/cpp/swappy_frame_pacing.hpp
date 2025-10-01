#pragma once

#include <cstdint>
#include <jni.h>
#include <EGL/egl.h>
#include <android/native_window.h>
#include <vector>
#include <mutex>
#include <algorithm>

// Forward declaration for SwappyStats
struct SwappyStats;

namespace mbgl {
namespace android {

/**
 * Frame timing statistics structure for native storage
 */
struct FrameTimingStats {
    // CPU timing statistics (nanoseconds)
    int64_t avgCpuTimeNs;
    int64_t minCpuTimeNs;
    int64_t maxCpuTimeNs;
    int64_t medianCpuTimeNs;

    // GPU timing statistics (nanoseconds)
    int64_t avgGpuTimeNs;
    int64_t minGpuTimeNs;
    int64_t maxGpuTimeNs;
    int64_t medianGpuTimeNs;

    // Total pipeline timing statistics (nanoseconds)
    int64_t avgTotalTimeNs;
    int64_t minTotalTimeNs;
    int64_t maxTotalTimeNs;
    int64_t medianTotalTimeNs;

    // Collection metadata
    int sampleCount;
    int64_t collectionDurationMs;

    // Constructor
    FrameTimingStats()
        : avgCpuTimeNs(0),
          minCpuTimeNs(0),
          maxCpuTimeNs(0),
          medianCpuTimeNs(0),
          avgGpuTimeNs(0),
          minGpuTimeNs(0),
          maxGpuTimeNs(0),
          medianGpuTimeNs(0),
          avgTotalTimeNs(0),
          minTotalTimeNs(0),
          maxTotalTimeNs(0),
          medianTotalTimeNs(0),
          sampleCount(0),
          collectionDurationMs(0) {}
};

/**
 * Wrapper class for Android Frame Pacing (Swappy) API.
 * Provides a C++ interface for integrating frame pacing into MapLibre's rendering pipeline.
 *
 * Based on the Android Frame Pacing documentation:
 * https://developer.android.com/games/sdk/frame-pacing/opengl/add-functions
 */
class SwappyFramePacing {
public:
    /**
     * Initialize the Swappy frame pacing library.
     * Should be called early in the application lifecycle.
     *
     * @param env JNI environment pointer
     * @param jactivity Java activity object
     * @return true if initialization was successful, false otherwise
     */
    static bool initialize(JNIEnv* env, jobject jactivity);

    /**
     * Destroy the Swappy frame pacing library.
     * Should be called when the application is exiting.
     */
    static void destroy();

    /**
     * Check if Swappy frame pacing is enabled and available.
     *
     * @return true if Swappy is enabled, false otherwise
     */
    static bool isEnabled();

    /**
     * Set the swap interval in nanoseconds.
     * Defines how long a frame should be presented.
     *
     * @param swapIntervalNs Swap interval in nanoseconds
     */
    static void setSwapInterval(uint64_t swapIntervalNs);

    /**
     * Set the swap interval using predefined frame rates.
     *
     * @param targetFps Target frame rate (e.g., 60, 30, 20)
     */
    static void setTargetFrameRate(int targetFps);

    /**
     * Set the fence timeout in nanoseconds.
     * Advanced configuration - usually not needed for basic integrations.
     *
     * @param fenceTimeoutNs Fence timeout in nanoseconds
     */
    static void setFenceTimeout(uint64_t fenceTimeoutNs);

    /**
     * Set whether to use CPU affinity.
     * Advanced configuration - usually not needed for basic integrations.
     *
     * @param useAffinity Whether to use CPU affinity
     */
    static void setUseAffinity(bool useAffinity);

    /**
     * Set the native window handle.
     * Should be called when the display surface changes.
     *
     * @param window Native window handle
     */
    static void setWindow(ANativeWindow* window);

    /**
     * Perform the frame swap operation.
     * This replaces eglSwapBuffers() calls in the rendering pipeline.
     *
     * @param display EGL display
     * @param surface EGL surface
     * @return true if swap was successful, false otherwise
     */
    static bool swap(EGLDisplay display, EGLSurface surface);

    // === METRICS AND STATISTICS ===

    /**
     * Enable or disable statistics collection.
     * When enabled, Swappy collects detailed frame timing metrics.
     *
     * @param enabled Whether to enable statistics collection
     */
    static void enableStats(bool enabled);

    static bool getStatsEnabled() { return sEnabled; }

    /**
     * Record the start of a frame for statistics collection.
     * Should be called at the beginning of each frame when stats are enabled.
     *
     * @param display EGL display
     * @param surface EGL surface
     */
    static void recordFrameStart(EGLDisplay display, EGLSurface surface);

    /**
     * Get collected frame statistics.
     * Provides detailed metrics about frame timing, drops, and compositor delays.
     *
     * @param stats Pointer to SwappyStats structure to be filled
     * @return true if stats were retrieved successfully, false otherwise
     */
    static bool getStats(SwappyStats* stats);

    /**
     * Clear all collected frame statistics.
     * Resets all counters to zero.
     */
    static void clearStats();

    // === ADVANCED CONFIGURATION ===

    /**
     * Set the buffer stuffing fix wait time.
     * This addresses GPU driver bugs where extra frames are "stuffed" into the pipeline.
     *
     * @param nFrames Number of bad frames to wait before applying the fix (0 = disabled)
     */
    static void setBufferStuffingFixWait(int32_t nFrames);

    /**
     * Enable or disable frame pacing entirely.
     * When disabled, frames are presented as soon as possible.
     *
     * @param enabled Whether to enable frame pacing
     */
    static void enableFramePacing(bool enabled);

    /**
     * Reset the frame pacing mechanism.
     * Clears frame timing history - useful after scene transitions or loading screens.
     */
    static void resetFramePacing();

    /**
     * Set maximum auto swap interval for adaptive frame timing.
     *
     * @param maxSwapIntervalNs Maximum swap interval in nanoseconds
     */
    static void setMaxAutoSwapInterval(uint64_t maxSwapIntervalNs);

    /**
     * Enable or disable auto swap interval adjustment.
     *
     * @param enabled Whether to enable automatic swap interval adjustment
     */
    static void setAutoSwapInterval(bool enabled);

    /**
     * Enable or disable auto pipeline mode.
     * Reduces latency by scheduling CPU and GPU work in the same pipeline stage.
     *
     * @param enabled Whether to enable auto pipeline mode
     */
    static void setAutoPipelineMode(bool enabled);

    /**
     * Log comprehensive frame statistics to help analyze performance.
     * This is a convenience method that retrieves and logs all available metrics.
     */
    static void logFrameStats();

    /**
     * Enable or disable frame timing callbacks.
     * When enabled, Swappy will call callbacks with CPU and GPU timing data.
     *
     * @param enabled Whether to enable frame timing callbacks
     */
    static void enableFrameTimingCallbacks(bool enabled);

    // === NATIVE FRAME TIMING STATISTICS ===

    /**
     * Enable or disable frame timing collection in native C++.
     * This replaces the Java callback mechanism with native storage.
     *
     * @param enabled Whether to enable native frame timing collection
     */
    static void enableNativeFrameTimingCollection(bool enabled);

    /**
     * Get current frame timing statistics computed in native C++.
     * Returns statistical analysis of CPU, GPU, and total pipeline timing.
     *
     * @param stats Pointer to FrameTimingStats structure to be filled
     * @return true if stats were retrieved successfully, false if no samples collected
     */
    static bool getNativeFrameTimingStats(FrameTimingStats* stats);

    /**
     * Clear native frame timing samples and reset collection.
     */
    static void clearNativeFrameTimingStats();

    /**
     * Add a native frame timing sample. Called internally by native callback.
     * This method is public to allow access from the global callback function.
     *
     * @param cpuTimeNs CPU processing time in nanoseconds
     * @param gpuTimeNs GPU processing time in nanoseconds (previous frame)
     */
    static void addNativeFrameTimingSample(int64_t cpuTimeNs, int64_t gpuTimeNs);

    // Common frame rate constants (in nanoseconds)
    static constexpr uint64_t FRAME_RATE_60FPS = 16666666ULL; // ~16.67ms
    static constexpr uint64_t FRAME_RATE_30FPS = 33333333ULL; // ~33.33ms
    static constexpr uint64_t FRAME_RATE_20FPS = 50000000ULL; // 50ms

private:
    static bool sInitialized;
    static bool sEnabled;

    // Native frame timing collection
    static constexpr int MAX_TIMING_SAMPLES = 1000;
    static bool sNativeTimingEnabled;
    static std::mutex sTimingMutex;
    static std::vector<int64_t> sCpuTimeSamples;
    static std::vector<int64_t> sGpuTimeSamples;
    static int64_t sCollectionStartTime;

    // Private helper methods
    static void calculateTimingStats(
        const std::vector<int64_t>& samples, int64_t& avg, int64_t& min, int64_t& max, int64_t& median);

    // Private constructor - this is a utility class with only static methods
    SwappyFramePacing() = delete;
    SwappyFramePacing(const SwappyFramePacing&) = delete;
    SwappyFramePacing& operator=(const SwappyFramePacing&) = delete;
};

} // namespace android
} // namespace mbgl
