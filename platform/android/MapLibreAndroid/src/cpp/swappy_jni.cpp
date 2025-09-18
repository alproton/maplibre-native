#include "swappy_frame_pacing.hpp"
#include <jni.h>
#include <mbgl/util/logging.hpp>
#include <swappy/swappy_common.h>

using namespace mbgl::android;

extern "C" {

/**
 * Initialize Swappy Frame Pacing from Java
 */
JNIEXPORT jboolean JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeInitialize(JNIEnv* env,
                                                                                                   jclass clazz,
                                                                                                   jobject activity) {
    return SwappyFramePacing::initialize(env, activity) ? JNI_TRUE : JNI_FALSE;
}

/**
 * Destroy Swappy Frame Pacing from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeDestroy(JNIEnv* env, jclass clazz) {
    SwappyFramePacing::destroy();
}

/**
 * Check if Swappy is enabled from Java
 */
JNIEXPORT jboolean JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeIsEnabled(JNIEnv* env,
                                                                                                  jclass clazz) {
    return SwappyFramePacing::isEnabled() ? JNI_TRUE : JNI_FALSE;
}

/**
 * Set target frame rate from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeSetTargetFrameRate(JNIEnv* env,
                                                                                                       jclass clazz,
                                                                                                       jint targetFps) {
    SwappyFramePacing::setTargetFrameRate(static_cast<int>(targetFps));
}

/**
 * Perform frame swap from Java
 */
JNIEXPORT jboolean JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeSwap(JNIEnv* env,
                                                                                             jclass clazz,
                                                                                             jlong display,
                                                                                             jlong surface) {
    EGLDisplay eglDisplay = reinterpret_cast<EGLDisplay>(display);
    EGLSurface eglSurface = reinterpret_cast<EGLSurface>(surface);
    return SwappyFramePacing::swap(eglDisplay, eglSurface) ? JNI_TRUE : JNI_FALSE;
}

// === METRICS AND STATISTICS JNI ===

/**
 * Enable or disable statistics collection from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeEnableStats(JNIEnv* env,
                                                                                                jclass clazz,
                                                                                                jboolean enabled) {
    SwappyFramePacing::enableStats(enabled == JNI_TRUE);
}

/**
 * Record frame start from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeRecordFrameStart(JNIEnv* env,
                                                                                                     jclass clazz,
                                                                                                     jlong display,
                                                                                                     jlong surface) {
    EGLDisplay eglDisplay = reinterpret_cast<EGLDisplay>(display);
    EGLSurface eglSurface = reinterpret_cast<EGLSurface>(surface);
    SwappyFramePacing::recordFrameStart(eglDisplay, eglSurface);
}

/**
 * Get frame statistics from Java
 * Returns a long array with statistics data
 */
JNIEXPORT jlongArray JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeGetStats(JNIEnv* env,
                                                                                                   jclass clazz) {
    SwappyStats stats;
    if (!SwappyFramePacing::getStats(&stats)) {
        return nullptr;
    }

    // Create array with: totalFrames + idleFrames[6] + lateFrames[6] + offsetFromPreviousFrame[6] + latencyFrames[6]
    // Total: 1 + 6 + 6 + 6 + 6 = 25 elements
    jlongArray result = env->NewLongArray(25);
    if (result == nullptr) {
        return nullptr;
    }

    jlong data[25];
    int index = 0;

    // Total frames
    data[index++] = static_cast<jlong>(stats.totalFrames);

    // Idle frames histogram (SurfaceFlinger delays)
    for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
        data[index++] = static_cast<jlong>(stats.idleFrames[i]);
    }

    // Late frames histogram (missed deadlines)
    for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
        data[index++] = static_cast<jlong>(stats.lateFrames[i]);
    }

    // Offset from previous frame histogram
    for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
        data[index++] = static_cast<jlong>(stats.offsetFromPreviousFrame[i]);
    }

    // Latency frames histogram
    for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
        data[index++] = static_cast<jlong>(stats.latencyFrames[i]);
    }

    env->SetLongArrayRegion(result, 0, 25, data);
    return result;
}

/**
 * Clear frame statistics from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeClearStats(JNIEnv* env,
                                                                                               jclass clazz) {
    SwappyFramePacing::clearStats();
}

/**
 * Log frame statistics from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeLogFrameStats(JNIEnv* env,
                                                                                                  jclass clazz) {
    SwappyFramePacing::logFrameStats();
}

// === ADVANCED CONFIGURATION JNI ===

/**
 * Set buffer stuffing fix wait from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeSetBufferStuffingFixWait(
    JNIEnv* env, jclass clazz, jint nFrames) {
    SwappyFramePacing::setBufferStuffingFixWait(static_cast<int32_t>(nFrames));
}

/**
 * Enable or disable frame pacing from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeEnableFramePacing(
    JNIEnv* env, jclass clazz, jboolean enabled) {
    SwappyFramePacing::enableFramePacing(enabled == JNI_TRUE);
}

/**
 * Reset frame pacing from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeResetFramePacing(JNIEnv* env,
                                                                                                     jclass clazz) {
    SwappyFramePacing::resetFramePacing();
}

/**
 * Set maximum auto swap interval from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeSetMaxAutoSwapInterval(
    JNIEnv* env, jclass clazz, jlong maxSwapIntervalNs) {
    SwappyFramePacing::setMaxAutoSwapInterval(static_cast<uint64_t>(maxSwapIntervalNs));
}

/**
 * Set auto swap interval from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeSetAutoSwapInterval(
    JNIEnv* env, jclass clazz, jboolean enabled) {
    SwappyFramePacing::setAutoSwapInterval(enabled == JNI_TRUE);
}

/**
 * Set auto pipeline mode from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeSetAutoPipelineMode(
    JNIEnv* env, jclass clazz, jboolean enabled) {
    SwappyFramePacing::setAutoPipelineMode(enabled == JNI_TRUE);
}

/**
 * Set CPU affinity from Java
 */
JNIEXPORT void JNICALL Java_org_maplibre_android_maps_renderer_SwappyRenderer_nativeSetUseAffinity(JNIEnv* env,
                                                                                                   jclass clazz,
                                                                                                   jboolean enabled) {
    SwappyFramePacing::setUseAffinity(enabled == JNI_TRUE);
}

} // extern "C"
