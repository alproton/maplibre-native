#include "swappy_frame_pacing.hpp"
#include <mbgl/util/logging.hpp>
#include <android/native_window_jni.h>

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

bool SwappyFramePacing::initialize(JNIEnv* env, jobject jactivity) {
    if (sInitialized) {
        Log::Warning(Event::OpenGL, "SwappyFramePacing already initialized");
        return sEnabled;
    }

    try {
        // Initialize Swappy
        SwappyGL_init(env, jactivity);

        // Check if Swappy is enabled
        sEnabled = SwappyGL_isEnabled();
        sInitialized = true;

        if (sEnabled) {
            Log::Info(Event::OpenGL, "SwappyFramePacing initialized successfully");

            // Set default configuration for optimal performance
            // Default to 60 FPS
            setTargetFrameRate(60);

            // Enable statistics collection for monitoring
            enableStats(true);
        } else {
            Log::Warning(Event::OpenGL, "SwappyFramePacing initialized but not enabled on this device");
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
        Log::Info(Event::OpenGL, "SwappyFramePacing destroyed");
    } catch (...) {
        Log::Error(Event::OpenGL, "Error during SwappyFramePacing destruction");
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
        Log::Debug(Event::OpenGL, "SwappyFramePacing not available, skipping setSwapInterval");
        return;
    }

    SwappyGL_setSwapIntervalNS(swapIntervalNs);
    Log::Debug(Event::OpenGL, "SwappyFramePacing swap interval set to " + std::to_string(swapIntervalNs) + " ns");
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
    Log::Info(Event::OpenGL, "SwappyFramePacing target frame rate set to " + std::to_string(targetFps) + " FPS");
}

void SwappyFramePacing::setFenceTimeout(uint64_t fenceTimeoutNs) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setFenceTimeoutNS(fenceTimeoutNs);
    Log::Debug(Event::OpenGL, "SwappyFramePacing fence timeout set to " + std::to_string(fenceTimeoutNs) + " ns");
}

void SwappyFramePacing::setUseAffinity(bool useAffinity) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setUseAffinity(useAffinity);
    Log::Debug(Event::OpenGL,
               std::string("SwappyFramePacing CPU affinity set to ") + (useAffinity ? "enabled" : "disabled"));
}

void SwappyFramePacing::setWindow(ANativeWindow* window) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setWindow(window);
    Log::Debug(Event::OpenGL, "SwappyFramePacing native window updated");
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
        Log::Warning(Event::OpenGL, "SwappyFramePacing not initialized, cannot enable stats");
        return;
    }

    SwappyGL_enableStats(enabled);
    Log::Info(Event::OpenGL, std::string("SwappyFramePacing statistics ") + (enabled ? "enabled" : "disabled"));
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
        Log::Error(Event::OpenGL, "Failed to retrieve SwappyFramePacing statistics");
        return false;
    }
}

void SwappyFramePacing::clearStats() {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_clearStats();
    Log::Debug(Event::OpenGL, "SwappyFramePacing statistics cleared");
}

// === ADVANCED CONFIGURATION ===

void SwappyFramePacing::setBufferStuffingFixWait(int32_t nFrames) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setBufferStuffingFixWait(nFrames);
    Log::Info(Event::OpenGL, "SwappyFramePacing buffer stuffing fix set to " + std::to_string(nFrames) + " frames");
}

void SwappyFramePacing::enableFramePacing(bool enabled) {
    if (!sInitialized) {
        return;
    }

    SwappyGL_enableFramePacing(enabled);
    Log::Info(Event::OpenGL, std::string("SwappyFramePacing frame pacing ") + (enabled ? "enabled" : "disabled"));
}

void SwappyFramePacing::resetFramePacing() {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_resetFramePacing();
    Log::Info(Event::OpenGL, "SwappyFramePacing frame timing history reset");
}

void SwappyFramePacing::setMaxAutoSwapInterval(uint64_t maxSwapIntervalNs) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setMaxAutoSwapIntervalNS(maxSwapIntervalNs);
    Log::Debug(Event::OpenGL,
               "SwappyFramePacing max auto swap interval set to " + std::to_string(maxSwapIntervalNs) + " ns");
}

void SwappyFramePacing::setAutoSwapInterval(bool enabled) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setAutoSwapInterval(enabled);
    Log::Debug(Event::OpenGL,
               std::string("SwappyFramePacing auto swap interval ") + (enabled ? "enabled" : "disabled"));
}

void SwappyFramePacing::setAutoPipelineMode(bool enabled) {
    if (!sInitialized || !sEnabled) {
        return;
    }

    SwappyGL_setAutoPipelineMode(enabled);
    Log::Debug(Event::OpenGL,
               std::string("SwappyFramePacing auto pipeline mode ") + (enabled ? "enabled" : "disabled"));
}

void SwappyFramePacing::logFrameStats() {
    if (!sInitialized || !sEnabled) {
        Log::Warning(Event::OpenGL, "SwappyFramePacing not available for statistics");
        return;
    }

    SwappyStats stats;
    if (!getStats(&stats)) {
        Log::Error(Event::OpenGL, "Failed to retrieve SwappyFramePacing statistics");
        return;
    }

    Log::Info(Event::OpenGL, "=== SwappyFramePacing Statistics ===");
    Log::Info(Event::OpenGL, "Total Frames: " + std::to_string(stats.totalFrames));

    // Analyze idle frames (frames waiting in compositor queue - indicates SurfaceFlinger delays)
    uint64_t totalIdleFrames = 0;
    for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
        totalIdleFrames += stats.idleFrames[i];
        if (stats.idleFrames[i] > 0) {
            Log::Info(
                Event::OpenGL,
                "Frames idle for " + std::to_string(i) + " refresh periods: " + std::to_string(stats.idleFrames[i]));
        }
    }

    // Analyze late frames (missed presentation deadlines)
    uint64_t totalLateFrames = 0;
    for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
        totalLateFrames += stats.lateFrames[i];
        if (stats.lateFrames[i] > 0) {
            Log::Info(Event::OpenGL,
                      "Frames " + std::to_string(i) + " refresh periods late: " + std::to_string(stats.lateFrames[i]));
        }
    }

    // Calculate performance percentages
    if (stats.totalFrames > 0) {
        double onTimePercentage = ((double)(stats.totalFrames - totalLateFrames) / stats.totalFrames) * 100.0;
        double compositorDelayPercentage = ((double)totalIdleFrames / stats.totalFrames) * 100.0;

        Log::Info(Event::OpenGL, "On-time frame delivery: " + std::to_string(onTimePercentage) + "%");
        Log::Info(Event::OpenGL, "Compositor delay impact: " + std::to_string(compositorDelayPercentage) + "%");
    }

    Log::Info(Event::OpenGL, "=== End Statistics ===");
}

} // namespace android
} // namespace mbgl
