package org.maplibre.android.maps.renderer.modern.surfaceview;

import android.content.Context;

import androidx.annotation.NonNull;

import org.maplibre.android.maps.renderer.modern.egl.EGLConfigChooser;
import org.maplibre.android.maps.renderer.modern.egl.EGLContextFactory;
import org.maplibre.android.maps.renderer.modern.egl.EGLWindowSurfaceFactory;
import org.maplibre.android.maps.renderer.surfaceview.SurfaceViewMapRenderer;

public class GLSurfaceViewMapRenderer extends SurfaceViewMapRenderer {

  public GLSurfaceViewMapRenderer(Context context,
                                @NonNull MapLibreGLSurfaceView surfaceView,
                                String localIdeographFontFamily) {
    super(context, surfaceView, localIdeographFontFamily);

    surfaceView.setEGLContextFactory(new EGLContextFactory());
    surfaceView.setEGLWindowSurfaceFactory(new EGLWindowSurfaceFactory());
    surfaceView.setEGLConfigChooser(new EGLConfigChooser());
    surfaceView.setRenderer(this);
    surfaceView.setRenderingRefreshMode(RenderingRefreshMode.WHEN_DIRTY);
    surfaceView.setPreserveEGLContextOnPause(true);
  }
}
