package org.maplibre.android.maps.renderer;

/**
 * Frame timing statistics containing CPU, GPU, and total pipeline timing data.
 * This Java class mirrors the native C++ FrameTimingStats structure.
 */
public class FrameTimingStats {
    // CPU timing statistics (nanoseconds)
    public final long avgCpuTimeNs;
    public final long minCpuTimeNs;
    public final long maxCpuTimeNs;
    public final long medianCpuTimeNs;

    // GPU timing statistics (nanoseconds)
    public final long avgGpuTimeNs;
    public final long minGpuTimeNs;
    public final long maxGpuTimeNs;
    public final long medianGpuTimeNs;

    // Total pipeline timing statistics (nanoseconds)
    public final long avgTotalTimeNs;
    public final long minTotalTimeNs;
    public final long maxTotalTimeNs;
    public final long medianTotalTimeNs;

    // Collection metadata
    public final int sampleCount;
    public final long collectionDurationMs;

    /**
     * Constructor that takes the native timing data array.
     * Array format: [avgCpuTimeNs, minCpuTimeNs, maxCpuTimeNs, medianCpuTimeNs,
     *                avgGpuTimeNs, minGpuTimeNs, maxGpuTimeNs, medianGpuTimeNs,
     *                avgTotalTimeNs, minTotalTimeNs, maxTotalTimeNs, medianTotalTimeNs,
     *                sampleCount, collectionDurationMs]
     */
    FrameTimingStats(long[] nativeStats) {
        if (nativeStats == null || nativeStats.length != 14) {
            throw new IllegalArgumentException("Invalid native stats array");
        }

        // CPU timing statistics
        this.avgCpuTimeNs = nativeStats[0];
        this.minCpuTimeNs = nativeStats[1];
        this.maxCpuTimeNs = nativeStats[2];
        this.medianCpuTimeNs = nativeStats[3];

        // GPU timing statistics
        this.avgGpuTimeNs = nativeStats[4];
        this.minGpuTimeNs = nativeStats[5];
        this.maxGpuTimeNs = nativeStats[6];
        this.medianGpuTimeNs = nativeStats[7];

        // Total pipeline timing statistics
        this.avgTotalTimeNs = nativeStats[8];
        this.minTotalTimeNs = nativeStats[9];
        this.maxTotalTimeNs = nativeStats[10];
        this.medianTotalTimeNs = nativeStats[11];

        // Collection metadata
        this.sampleCount = (int) nativeStats[12];
        this.collectionDurationMs = nativeStats[13];
    }

    @Override
    public String toString() {
        if (sampleCount == 0) {
            return "FrameTimingStats{no samples collected}";
        }

        return String.format(
                "FrameTimingStats{\n" +
                        "  samples=%d, duration=%dms\n" +
                        "  CPU: avg=%.2fms, min=%.2fms, max=%.2fms, median=%.2fms\n" +
                        "  GPU: avg=%.2fms, min=%.2fms, max=%.2fms, median=%.2fms\n" +
                        "  Total: avg=%.2fms, min=%.2fms, max=%.2fms, median=%.2fms\n" +
                        "}",
                sampleCount, collectionDurationMs,
                avgCpuTimeNs / 1_000_000.0, minCpuTimeNs / 1_000_000.0,
                maxCpuTimeNs / 1_000_000.0, medianCpuTimeNs / 1_000_000.0,
                avgGpuTimeNs / 1_000_000.0, minGpuTimeNs / 1_000_000.0,
                maxGpuTimeNs / 1_000_000.0, medianGpuTimeNs / 1_000_000.0,
                avgTotalTimeNs / 1_000_000.0, minTotalTimeNs / 1_000_000.0,
                maxTotalTimeNs / 1_000_000.0, medianTotalTimeNs / 1_000_000.0
        );
    }

    /**
     * Get a compact single-line summary of the timing statistics.
     *
     * @return Compact timing summary string
     */
    public String toCompactString() {
        if (sampleCount == 0) {
            return "FrameTimingStats{no samples}";
        }

        return String.format(
                "FrameTimingStats{samples=%d, CPU=%.2fms, GPU=%.2fms, Total=%.2fms}",
                sampleCount,
                avgCpuTimeNs / 1_000_000.0,
                avgGpuTimeNs / 1_000_000.0,
                avgTotalTimeNs / 1_000_000.0
        );
    }
}
