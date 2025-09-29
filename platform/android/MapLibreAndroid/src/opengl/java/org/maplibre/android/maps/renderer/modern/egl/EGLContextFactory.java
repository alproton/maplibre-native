package org.maplibre.android.maps.renderer.modern.egl;

import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.GLSurfaceView;
import android.util.Log;

import androidx.annotation.Nullable;


public class EGLContextFactory {

  public EGLContext createContext(@Nullable EGLDisplay display, @Nullable EGLConfig config) {
    if (display == null || config == null) {
      return EGL14.EGL_NO_CONTEXT;
    }

    final int[] contextAttributes = {
            EGL14.EGL_CONTEXT_CLIENT_VERSION, 3, // Request an OpenGL ES 3.0 context
            EGL14.EGL_NONE // This terminates the attribute list
    };

    return EGL14.eglCreateContext(display, config, EGL14.EGL_NO_CONTEXT, contextAttributes, 0);
  }

  public void destroyContext(EGLDisplay display, EGLContext context) {
    if (!EGL14.eglDestroyContext(display, context)) {
      Log.e("DefaultContextFactory", "display:" + display + " context: " + context);
      Log.i("DefaultContextFactory", "tid=" + Thread.currentThread().getId());
    }
  }
}
