package org.maplibre.android.maps.renderer.modern.egl;

import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.opengl.GLSurfaceView;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;


public class EGLWindowSurfaceFactory {
  public EGLSurface createWindowSurface(@Nullable EGLDisplay display, @Nullable EGLConfig config,
                                        @Nullable Object nativeWindow, int[] attrib_list, int offset) {
    EGLSurface result = null;
    if (display != null && config != null && nativeWindow != null) {
      try {
        result = EGL14.eglCreateWindowSurface(display, config, nativeWindow, attrib_list, offset);
      } catch (Exception exception) {
        // This exception indicates that the surface flinger surface
        // is not valid. This can happen if the surface flinger surface has
        // been torn down, but the application has not yet been
        // notified via SurfaceHolder.Callback.surfaceDestroyed.
        // In theory the application should be notified first,
        // but in practice sometimes it is not. See b/4588890
        Log.e("EGLWindowSurfaceFactory", "eglCreateWindowSurface", exception);
      }
    }
    return result;
  }

  public void destroySurface(EGLDisplay display, EGLSurface surface) {
    EGL14.eglDestroySurface(display, surface);
  }
}
