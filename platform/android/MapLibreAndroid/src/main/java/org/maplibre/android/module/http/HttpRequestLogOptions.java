package org.maplibre.android.module.http;

import androidx.annotation.NonNull;

/**
 * Configuration options for native HTTP tile request logging.
 *
 * <p>Use with {@link HttpRequestUtil#setHttpRequestLogOptions(HttpRequestLogOptions)}
 * to control what gets logged.</p>
 *
 * <ul>
 *   <li>{@link HttpRequestLogLevel#VERBOSE} — logs every individual tile download
 *       (elapsed time, URL, status, size). The {@code durationSeconds} field is ignored.</li>
 *   <li>{@link HttpRequestLogLevel#STATS} — accumulates tile download statistics and
 *       logs a summary (total, success, failed counts and min/max/median elapsed times)
 *       every {@code durationSeconds} seconds.</li>
 * </ul>
 */
public class HttpRequestLogOptions {

  @NonNull
  private final HttpRequestLogLevel level;
  private final long durationSeconds;

  /**
   * Create log options.
   *
   * @param level           the logging granularity
   * @param durationSeconds the stats window duration in seconds. Only used when
   *                        level is {@link HttpRequestLogLevel#STATS}; ignored for VERBOSE.
   *                        Must be at least 1 when using STATS.
   */
  public HttpRequestLogOptions(@NonNull HttpRequestLogLevel level, long durationSeconds) {
    this.level = level;
    this.durationSeconds = durationSeconds;
  }

  /**
   * Convenience constructor for {@link HttpRequestLogLevel#VERBOSE} mode.
   *
   * @param level the logging granularity
   */
  public HttpRequestLogOptions(@NonNull HttpRequestLogLevel level) {
    this(level, 0);
  }

  @NonNull
  public HttpRequestLogLevel getLevel() {
    return level;
  }

  /**
   * @return the stats window duration in seconds
   */
  public long getDurationSeconds() {
    return durationSeconds;
  }
}
