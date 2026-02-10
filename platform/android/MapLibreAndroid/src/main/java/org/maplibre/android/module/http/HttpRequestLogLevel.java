package org.maplibre.android.module.http;

/**
 * Controls the granularity of native HTTP tile request logging.
 */
public enum HttpRequestLogLevel {
  /**
   * Log the elapsed time and details for every individual tile download.
   */
  VERBOSE,

  /**
   * Log aggregate statistics (total, success, failed counts and min/max/median
   * elapsed times) over a configurable duration window.
   */
  STATS
}
