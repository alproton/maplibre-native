#include "swappy_frame_pacing.hpp"
#include <mbgl/util/logging.hpp>
#include <android/native_window_jni.h>
#include <chrono>
#include "../../../src/attach_env.hpp"

// Include Swappy headers
#include <swappy/swappyGL.h>
#include <swappy/swappyGL_extra.h>
#include <swappy/swappy_common.h>

// Ensure MAX_FRAME_BUCKETS is defined (from swappy_common.h)
#ifndef MAX_FRAME_BUCKETS
#define MAX_FRAME_BUCKETS 6
#endif

namespace mbgl {
namespace android {

// Static member initialization
bool SwappyFramePacing::sInitialized = false;
bool SwappyFramePacing::sEnabled = false;

// Native frame timing collection static members
bool SwappyFramePacing::sNativeTimingEnabled = false;
std::mutex SwappyFramePacing::sTimingMutex;
std::vector<int64_t> SwappyFramePacing::sCpuTimeSamples;
std::vector<int64_t> SwappyFramePacing::sGpuTimeSamples;
int64_t SwappyFramePacing::sCollectionStartTime = 0;

bool SwappyFramePacing::initialize(JNIEnv* env, jobject jactivity) {
    if (sInitialized) {
        Log::Warning(Event::Swappy, "SwappyFramePacing already initialized");
        return sEnabled;
    }

    try {
        // Initialize Swappy
        SwappyGL_init(env, jactivity);

        // Check if Swappy is enabled
        sEnabled = SwappyGL_isEnabled();
        sInitialized = true;

        if (sEnabled) {
            Log::Info(Event::Swappy, "SwappyFramePacing initialized successfully");

            // Set default configuration for optimal performance
            // Default to 60 FPS
            setTargetFrameRate(60);

            // Enable statistics collection for monitoring
            enableStats(true);
        } else {
            Log::Warning(Event::Swappy, "SwappyFramePacing initialized but not enabled on this device");
        }

        return sEnabled;
    } catch (...) {
        Log::Error(Event::OpenGL, "Failed to initialize SwappyFramePacing");
        sInitialized = false;
        sEnabled = false;
        return false;
    }
}

void SwappyFramePacing::destroy() {
    if (!sInitialized) {
        return;
    }

    try {
        SwappyGL_destroy();
        Log::Info(Event::Swappy, "SwappyFramePacing destroyed");
    } catch (...) {
        Log::Error(Event::Swappy, "Error during SwappyFramePacing destruction");
    }

    sInitialized = false;
    sEnabled = false;
}

bool SwappyFramePacing::isEnabled() {
    if (!sInitialized) {
        return false;
    }
    return SwappyGL_isEnabled();
}

void SwappyFramePacing::setSwapInterval(uint64_t swapIntervalNs) {
    if (!sInitialized || !sEnabled) {
        Log::Debug(Event::Swappy, "SwappyFramePacing not available, skipping setSwapInterval");
        return;
    }

    SwappyGL_setSwapIntervalNS(swapIntervalNs);
    Log::Debug(Event::Swappy, "SwappyFramePacing swap interval set to " + std::to_string(swapIntervalNs) + " ns");
}

void SwappyFramePacing::setTargetFrameRate(int targetFps) {
    uint64_t swapIntervalNs;

    switch (targetFps) {
        case 60:
            swapIntervalNs = FRAME_RATE_60FPS;
            break;
        case 30:
            swapIntervalNs = FRAME_RATE_30FPS;
            break;
        case 20:
            swapIntervalNs = FRAME_RATE_20FPS;
            break;
        default:
            // Calculate interval for custom frame rate
            swapIntervalNs = 1000000000ULL / targetFps;
            break;
    }

    setSwapInterval(swapIntervalNs);
    Log::Info(Event::Swappy, "SwappyFramePacing target frame rate set to " + std::to_string(targetFps) + " FPS");
}

void SwappyFramePacing::setFenceTimeout(uint64_t fenceTimeoutNs) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setFenceTimeoutNS(fenceTimeoutNs);
    Log::Debug(Event::Swappy, "SwappyFramePacing fence timeout set to " + std::to_string(fenceTimeoutNs) + " ns");
}

void SwappyFramePacing::setUseAffinity(bool useAffinity) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setUseAffinity(useAffinity);
    Log::Debug(Event::Swappy,
               std::string("SwappyFramePacing CPU affinity set to ") + (useAffinity ? "enabled" : "disabled"));
}

void SwappyFramePacing::setWindow(ANativeWindow* window) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setWindow(window);
    Log::Debug(Event::Swappy, "SwappyFramePacing native window updated");
}

bool SwappyFramePacing::swap(EGLDisplay display, EGLSurface surface) {
    if (!sInitialized) {
        // Fall back to standard eglSwapBuffers if Swappy is not initialized
        return eglSwapBuffers(display, surface) == EGL_TRUE;
    }

    // SwappyGL_swap handles the case where Swappy is not enabled by falling back to eglSwapBuffers
    return SwappyGL_swap(display, surface);
}

// === METRICS AND STATISTICS ===

void SwappyFramePacing::enableStats(bool enabled) {
    if (!sInitialized) {
        Log::Warning(Event::Swappy, "SwappyFramePacing not initialized, cannot enable stats");
        return;
    }

    SwappyGL_enableStats(enabled);
    Log::Info(Event::Swappy, std::string("SwappyFramePacing statistics ") + (enabled ? "enabled" : "disabled"));
}

void SwappyFramePacing::recordFrameStart(EGLDisplay display, EGLSurface surface) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_recordFrameStart(display, surface);
}

bool SwappyFramePacing::getStats(SwappyStats* stats) {
    if (!sInitialized || !sEnabled || !stats) {
        return false;
    }

    try {
        SwappyGL_getStats(stats);
        return true;
    } catch (...) {
        Log::Error(Event::Swappy, "Failed to retrieve SwappyFramePacing statistics");
        return false;
    }
}

void SwappyFramePacing::clearStats() {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_clearStats();
    Log::Debug(Event::Swappy, "SwappyFramePacing statistics cleared");
}

// === ADVANCED CONFIGURATION ===

void SwappyFramePacing::setBufferStuffingFixWait(int32_t nFrames) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setBufferStuffingFixWait(nFrames);
    Log::Info(Event::Swappy, "SwappyFramePacing buffer stuffing fix set to " + std::to_string(nFrames) + " frames");
}

void SwappyFramePacing::enableFramePacing(bool enabled) {
    if (!sInitialized) {
        return;
    }

    SwappyGL_enableFramePacing(enabled);
    Log::Info(Event::Swappy, std::string("SwappyFramePacing frame pacing ") + (enabled ? "enabled" : "disabled"));
}

void SwappyFramePacing::resetFramePacing() {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_resetFramePacing();
    Log::Info(Event::Swappy, "SwappyFramePacing frame timing history reset");
}

void SwappyFramePacing::setMaxAutoSwapInterval(uint64_t maxSwapIntervalNs) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setMaxAutoSwapIntervalNS(maxSwapIntervalNs);
    Log::Debug(Event::Swappy,
               "SwappyFramePacing max auto swap interval set to " + std::to_string(maxSwapIntervalNs) + " ns");
}

void SwappyFramePacing::setAutoSwapInterval(bool enabled) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setAutoSwapInterval(enabled);
    Log::Debug(Event::Swappy,
               std::string("SwappyFramePacing auto swap interval ") + (enabled ? "enabled" : "disabled"));
}

void SwappyFramePacing::setAutoPipelineMode(bool enabled) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setAutoPipelineMode(enabled);
    Log::Debug(Event::Swappy,
               std::string("SwappyFramePacing auto pipeline mode ") + (enabled ? "enabled" : "disabled"));
}

void SwappyFramePacing::logFrameStats() {
    if (!sInitialized || !sEnabled) {
        Log::Warning(Event::Swappy, "SwappyFramePacing not available for statistics");
        return;
    }

    SwappyStats stats;
    if (!getStats(&stats)) {
        Log::Error(Event::Swappy, "Failed to retrieve SwappyFramePacing statistics");
        return;
    }

    Log::Info(Event::Swappy, "=== SwappyFramePacing Statistics ===");
    Log::Info(Event::Swappy, "Total Frames: " + std::to_string(stats.totalFrames));

    // Analyze idle frames (frames waiting in compositor queue - indicates SurfaceFlinger delays)
    uint64_t totalIdleFrames = 0;
    for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
        totalIdleFrames += stats.idleFrames[i];
        if (stats.idleFrames[i] > 0) {
            Log::Info(
                Event::Swappy,
                "Frames idle for " + std::to_string(i) + " refresh periods: " + std::to_string(stats.idleFrames[i]));
        }
    }

    // Analyze late frames (missed presentation deadlines)
    uint64_t totalLateFrames = 0;
    for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
        totalLateFrames += stats.lateFrames[i];
        if (stats.lateFrames[i] > 0) {
            Log::Info(Event::Swappy,
                      "Frames " + std::to_string(i) + " refresh periods late: " + std::to_string(stats.lateFrames[i]));
        }
    }

    // Calculate performance percentages
    if (stats.totalFrames > 0) {
        double onTimePercentage = ((double)(stats.totalFrames - totalLateFrames) / stats.totalFrames) * 100.0;
        double compositorDelayPercentage = ((double)totalIdleFrames / stats.totalFrames) * 100.0;

        Log::Info(Event::Swappy, "On-time frame delivery: " + std::to_string(onTimePercentage) + "%");
        Log::Info(Event::Swappy, "Compositor delay impact: " + std::to_string(compositorDelayPercentage) + "%");
    }

    Log::Info(Event::Swappy, "=== End Statistics ===");
}

// === NATIVE FRAME TIMING COLLECTION ===

namespace {
// Native callback function for frame timing collection
void swappyPostWaitCallback(void* userData, int64_t cpuTimeNs, int64_t gpuTimeNs) {
    // Store timing data directly in native C++ - no JNI calls needed
    if (cpuTimeNs >= 0 && gpuTimeNs >= 0) {
        SwappyFramePacing::addNativeFrameTimingSample(cpuTimeNs, gpuTimeNs);
    }
}
} // namespace

void SwappyFramePacing::enableFrameTimingCallbacks(bool enabled) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    if (enabled) {
        // Set up tracer with our callback
        SwappyTracer tracer = {};
        tracer.postWait = swappyPostWaitCallback;
        tracer.userData = nullptr;

        SwappyGL_injectTracer(&tracer);
        Log::Info(Event::OpenGL, "SwappyFramePacing timing callbacks enabled");
    } else {
        // Remove tracer callbacks
        SwappyTracer tracer = {};
        tracer.postWait = nullptr; // Clear the callback
        tracer.userData = nullptr;

        SwappyGL_uninjectTracer(&tracer);
        Log::Info(Event::OpenGL, "SwappyFramePacing timing callbacks disabled");
    }
}

// === NATIVE FRAME TIMING IMPLEMENTATION ===

void SwappyFramePacing::enableNativeFrameTimingCollection(bool enabled) {
    std::lock_guard<std::mutex> lock(sTimingMutex);

    if (enabled && !sNativeTimingEnabled) {
        // Enable native timing collection
        sNativeTimingEnabled = true;
        sCpuTimeSamples.clear();
        sGpuTimeSamples.clear();
        sCpuTimeSamples.reserve(MAX_TIMING_SAMPLES);
        sGpuTimeSamples.reserve(MAX_TIMING_SAMPLES);
        sCollectionStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count();

        // Enable the Swappy callbacks
        enableFrameTimingCallbacks(true);

        Log::Info(Event::Swappy, "Native frame timing collection enabled");
    } else if (!enabled && sNativeTimingEnabled) {
        // Disable native timing collection
        sNativeTimingEnabled = false;

        // Disable the Swappy callbacks
        enableFrameTimingCallbacks(false);

        Log::Info(Event::Swappy, "Native frame timing collection disabled");
    }
}

void SwappyFramePacing::addNativeFrameTimingSample(int64_t cpuTimeNs, int64_t gpuTimeNs) {
    if (!sNativeTimingEnabled) {
        return;
    }

    std::lock_guard<std::mutex> lock(sTimingMutex);

    // Add samples to rolling buffer
    if (sCpuTimeSamples.size() >= MAX_TIMING_SAMPLES) {
        // Remove oldest sample
        sCpuTimeSamples.erase(sCpuTimeSamples.begin());
        sGpuTimeSamples.erase(sGpuTimeSamples.begin());
    }

    sCpuTimeSamples.push_back(cpuTimeNs);
    sGpuTimeSamples.push_back(gpuTimeNs);
}

bool SwappyFramePacing::getNativeFrameTimingStats(FrameTimingStats* stats) {
    if (!stats || !sNativeTimingEnabled) {
        return false;
    }

    std::lock_guard<std::mutex> lock(sTimingMutex);

    if (sCpuTimeSamples.empty()) {
        return false;
    }

    // Calculate CPU timing statistics
    calculateTimingStats(
        sCpuTimeSamples, stats->avgCpuTimeNs, stats->minCpuTimeNs, stats->maxCpuTimeNs, stats->medianCpuTimeNs);

    // Calculate GPU timing statistics
    calculateTimingStats(
        sGpuTimeSamples, stats->avgGpuTimeNs, stats->minGpuTimeNs, stats->maxGpuTimeNs, stats->medianGpuTimeNs);

    // Calculate total pipeline timing statistics
    std::vector<int64_t> totalTimes;
    totalTimes.reserve(sCpuTimeSamples.size());
    for (size_t i = 0; i < sCpuTimeSamples.size(); ++i) {
        totalTimes.push_back(sCpuTimeSamples[i] + sGpuTimeSamples[i]);
    }
    calculateTimingStats(
        totalTimes, stats->avgTotalTimeNs, stats->minTotalTimeNs, stats->maxTotalTimeNs, stats->medianTotalTimeNs);

    // Set collection metadata
    stats->sampleCount = static_cast<int>(sCpuTimeSamples.size());
    int64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count();
    stats->collectionDurationMs = currentTime - sCollectionStartTime;

    return true;
}

void SwappyFramePacing::clearNativeFrameTimingStats() {
    std::lock_guard<std::mutex> lock(sTimingMutex);

    sCpuTimeSamples.clear();
    sGpuTimeSamples.clear();
    sCollectionStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

    Log::Info(Event::Swappy, "Native frame timing statistics cleared");
}

void SwappyFramePacing::calculateTimingStats(
    const std::vector<int64_t>& samples, int64_t& avg, int64_t& min, int64_t& max, int64_t& median) {
    if (samples.empty()) {
        avg = min = max = median = 0;
        return;
    }

    // Calculate average
    int64_t sum = 0;
    for (int64_t sample : samples) {
        sum += sample;
    }
    avg = sum / static_cast<int64_t>(samples.size());

    // Find min and max
    auto minmax = std::minmax_element(samples.begin(), samples.end());
    min = *minmax.first;
    max = *minmax.second;

    // Calculate median
    std::vector<int64_t> sortedSamples = samples;
    std::sort(sortedSamples.begin(), sortedSamples.end());
    size_t size = sortedSamples.size();
    if (size % 2 == 0) {
        median = (sortedSamples[size / 2 - 1] + sortedSamples[size / 2]) / 2;
    } else {
        median = sortedSamples[size / 2];
    }
}

} // namespace android
} // namespace mbgl
