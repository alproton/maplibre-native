package org.maplibre.android.module.http;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import okhttp3.Call;
import okio.Buffer;

/**
 * Utility class for setting OkHttpRequest configurations
 */
public class HttpRequestUtil {

  /**
   * Set the log state of OkHttpRequest. Default value is true.
   * <p>
   * This configuration will outlast the lifecycle of the Map.
   * </p>
   *
   * @param enabled True will enable logging, false will disable
   */
  public static void setLogEnabled(boolean enabled) {
    HttpRequestImpl.enableLog(enabled);
  }

  /**
   * Enable printing of the request url when an error occurred. Default value is false.
   * <p>
   * Requires {@link #setLogEnabled(boolean)} to be activated.
   * </p>
   * <p>
   * This configuration will outlast the lifecycle of the Map.
   * </p>
   *
   * @param enabled True will print urls, false will disable
   */
  public static void setPrintRequestUrlOnFailure(boolean enabled) {
    HttpRequestImpl.enablePrintRequestUrlOnFailure(enabled);
  }

  /**
   * Set the OkHttp Call.Factory used for requesting map resources.
   * <p>
   * This configuration survives across mapView instances.
   * Reset the OkHttpClient to the default by passing null as parameter.
   * </p>
   *
   * @param client the OkHttp Call.Factory, typically OkHttpClient.
   */
  public static void setOkHttpClient(@Nullable Call.Factory client) {
    HttpRequestImpl.setOkHttpClient(client);
  }

  /**
   * Enable or disable native HTTP tile request timing logs.
   * <p>
   * When enabled, every individual tile download is logged with its URL, HTTP status,
   * response size and elapsed time (equivalent to {@link HttpRequestLogLevel#VERBOSE}).
   * When disabled, native tile request logging is turned off.
   * </p>
   * <p>
   * This configuration will outlast the lifecycle of the Map.
   * </p>
   *
   * @param enabled true to enable per-tile timing logs, false to disable
   */
  public static void setTimingLogsEnabled(boolean enabled) {
    nativeSetTimingLogsEnabled(enabled);
  }

  /**
   * Configure native HTTP tile request logging.
   * <p>
   * This configuration will outlast the lifecycle of the Map.
   * </p>
   *
   * @param options the log options
   * @see HttpRequestLogOptions
   * @see HttpRequestLogLevel
   */
  public static void setHttpRequestLogOptions(@NonNull HttpRequestLogOptions options) {
    nativeSetHttpRequestLogOptions(options.getLevel().ordinal(), options.getDurationSeconds());
  }

  @Keep
  private static native void nativeSetTimingLogsEnabled(boolean enabled);

  @Keep
  private static native void nativeSetHttpRequestLogOptions(int level, long durationSeconds);

  @NonNull
  static String toHumanReadableAscii(String s) {
    for (int i = 0, length = s.length(), c; i < length; i += Character.charCount(c)) {
      c = s.codePointAt(i);
      if (c > '\u001f' && c < '\u007f') {
        continue;
      }

      Buffer buffer = new Buffer();
      buffer.writeUtf8(s, 0, i);
      for (int j = i; j < length; j += Character.charCount(c)) {
        c = s.codePointAt(j);
        buffer.writeUtf8CodePoint(c > '\u001f' && c < '\u007f' ? c : '?');
      }
      return buffer.readUtf8();
    }
    return s;
  }
}
