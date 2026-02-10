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
   * Enable or disable logging of elapsed time for native HTTP tile requests.
   * When enabled, the native layer logs the round-trip duration for each tile
   * download at Info severity, or Warning severity on failure.
   * <p>
   * Default value is false. This configuration will outlast the lifecycle of the Map.
   * </p>
   *
   * @param enabled True will enable timing logs, false will disable
   */
  public static void setTimingLogsEnabled(boolean enabled) {
    nativeSetTimingLogsEnabled(enabled);
  }

  /**
   * Returns whether native HTTP tile request timing logs are enabled.
   *
   * @return True if timing logs are enabled, false otherwise
   */
  public static boolean isTimingLogsEnabled() {
    return nativeIsTimingLogsEnabled();
  }

  @Keep
  private static native void nativeSetTimingLogsEnabled(boolean enabled);

  @Keep
  private static native boolean nativeIsTimingLogsEnabled();

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
