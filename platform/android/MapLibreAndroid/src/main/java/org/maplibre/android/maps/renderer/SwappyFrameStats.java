package org.maplibre.android.maps.renderer;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * SwappyFrameStats provides detailed frame timing and performance metrics
 * collected by the Android Frame Pacing library (Swappy).
 *
 * This class helps analyze rendering performance, identify frame drops,
 * and detect SurfaceFlinger/compositor delays.
 */
@Keep
public class SwappyFrameStats {

    private static final int MAX_FRAME_BUCKETS = 6;

    private final long totalFrames;
    private final long[] idleFrames;
    private final long[] lateFrames;
    private final long[] offsetFromPreviousFrame;
    private final long[] latencyFrames;

    /**
     * Package-private constructor used by SwappyRenderer.
     *
     * @param rawStats Raw statistics array from native code
     */
    SwappyFrameStats(@NonNull long[] rawStats) {
        if (rawStats.length != 25) {
            throw new IllegalArgumentException("Invalid stats array length: " + rawStats.length);
        }

        int index = 0;

        // Parse total frames
        totalFrames = rawStats[index++];

        // Parse idle frames histogram
        idleFrames = new long[MAX_FRAME_BUCKETS];
        for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
            idleFrames[i] = rawStats[index++];
        }

        // Parse late frames histogram
        lateFrames = new long[MAX_FRAME_BUCKETS];
        for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
            lateFrames[i] = rawStats[index++];
        }

        // Parse offset from previous frame histogram
        offsetFromPreviousFrame = new long[MAX_FRAME_BUCKETS];
        for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
            offsetFromPreviousFrame[i] = rawStats[index++];
        }

        // Parse latency frames histogram
        latencyFrames = new long[MAX_FRAME_BUCKETS];
        for (int i = 0; i < MAX_FRAME_BUCKETS; i++) {
            latencyFrames[i] = rawStats[index++];
        }
    }

    /**
     * Get the total number of frames processed by Swappy.
     *
     * @return Total frame count
     */
    public long getTotalFrames() {
        return totalFrames;
    }

    /**
     * Get histogram of frames that waited in the compositor queue.
     * This directly indicates SurfaceFlinger delays!
     *
     * @return Array where index represents refresh periods waited, value is frame count
     */
    @NonNull
    public long[] getIdleFrames() {
        return idleFrames.clone();
    }

    /**
     * Get histogram of frames that were presented late.
     * Indicates rendering performance issues.
     *
     * @return Array where index represents refresh periods late, value is frame count
     */
    @NonNull
    public long[] getLateFrames() {
        return lateFrames.clone();
    }

    /**
     * Get histogram of time between consecutive frames.
     * Helps analyze frame pacing consistency.
     *
     * @return Array where index represents refresh periods between frames, value is frame count
     */
    @NonNull
    public long[] getOffsetFromPreviousFrame() {
        return offsetFromPreviousFrame.clone();
    }

    /**
     * Get histogram of end-to-end frame latency.
     * Useful for input responsiveness analysis.
     *
     * @return Array where index represents refresh periods of total latency, value is frame count
     */
    @NonNull
    public long[] getLatencyFrames() {
        return latencyFrames.clone();
    }

    /**
     * Calculate the percentage of frames delivered on time.
     *
     * @return Percentage of frames that met their presentation deadline
     */
    public double getOnTimeFramePercentage() {
        if (totalFrames == 0) return 0.0;

        long totalLateFrames = 0;
        for (long count : lateFrames) {
            totalLateFrames += count;
        }

        return ((double)(totalFrames - totalLateFrames) / totalFrames) * 100.0;
    }

    /**
     * Calculate the percentage of frames affected by compositor delays.
     * This indicates SurfaceFlinger performance impact.
     *
     * @return Percentage of frames that experienced compositor delays
     */
    public double getCompositorDelayPercentage() {
        if (totalFrames == 0) return 0.0;

        long totalIdleFrames = 0;
        for (long count : idleFrames) {
            totalIdleFrames += count;
        }

        return ((double)totalIdleFrames / totalFrames) * 100.0;
    }

    /**
     * Get the total number of frames that experienced compositor delays.
     *
     * @return Count of frames that waited in the compositor queue
     */
    public long getTotalIdleFrames() {
        long total = 0;
        for (long count : idleFrames) {
            total += count;
        }
        return total;
    }

    /**
     * Get the total number of frames that were presented late.
     *
     * @return Count of frames that missed their presentation deadline
     */
    public long getTotalLateFrames() {
        long total = 0;
        for (long count : lateFrames) {
            total += count;
        }
        return total;
    }

    /**
     * Get the average frame consistency score (0-100).
     * Higher scores indicate more consistent frame timing.
     *
     * @return Frame consistency score as a percentage
     */
    public double getFrameConsistencyScore() {
        if (totalFrames == 0) return 0.0;

        // Frames with 0 or 1 refresh period offset are considered consistent
        long consistentFrames = 0;
        if (offsetFromPreviousFrame.length > 0) consistentFrames += offsetFromPreviousFrame[0];
        if (offsetFromPreviousFrame.length > 1) consistentFrames += offsetFromPreviousFrame[1];

        return ((double)consistentFrames / totalFrames) * 100.0;
    }

    @Override
    @NonNull
    public String toString() {
        return String.format(
            "SwappyFrameStats{totalFrames=%d, onTime=%.1f%%, compositorDelay=%.1f%%, consistency=%.1f%%}",
            totalFrames,
            getOnTimeFramePercentage(),
            getCompositorDelayPercentage(),
            getFrameConsistencyScore()
        );
    }
}
