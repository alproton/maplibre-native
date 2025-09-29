package org.maplibre.android.maps.renderer;

import android.content.Context;
import android.view.Surface;
import android.view.TextureView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import org.maplibre.android.maps.renderer.surfaceview.SurfaceViewMapRenderer;
import org.maplibre.android.maps.renderer.textureview.GLTextureViewRenderThread;
import org.maplibre.android.maps.renderer.textureview.TextureViewMapRenderer;

@Keep
public class MapRendererFactory {
  public static TextureViewMapRenderer newTextureViewMapRenderer(@NonNull Context context, TextureView textureView,
                                                                 String localFontFamily, boolean translucentSurface,
                                                                 Runnable initCallback, int threadPriorityOverride) {

    TextureViewMapRenderer mapRenderer = new TextureViewMapRenderer(context, textureView,
            localFontFamily, translucentSurface, threadPriorityOverride) {
      @Override
      public void onSurfaceCreated(Surface surface) {
        initCallback.run();
        super.onSurfaceCreated(surface);
      }
    };

    mapRenderer.setRenderThread(new GLTextureViewRenderThread(textureView, mapRenderer));
    return mapRenderer;
  }

  public static SurfaceViewMapRenderer newSurfaceViewMapRenderer(@NonNull Context context, String localFontFamily,
                                                                 boolean renderSurfaceOnTop, Runnable initCallback,
                                                                 int threadPriorityOverride, boolean useModernEGL) {

    if(useModernEGL) {
      org.maplibre.android.maps.renderer.modern.surfaceview.MapLibreGLSurfaceView surfaceView = new org.maplibre.android.maps.renderer.modern.surfaceview.MapLibreGLSurfaceView(context);
      surfaceView.setZOrderMediaOverlay(renderSurfaceOnTop);

      return new org.maplibre.android.maps.renderer.modern.surfaceview.GLSurfaceViewMapRenderer(context, surfaceView, localFontFamily, threadPriorityOverride) {
        @Override
        public void onSurfaceCreated(Surface surface) {
          initCallback.run();
          super.onSurfaceCreated(surface);
        }
      };
    }
    //Swappy is not supported for the legacy EGL path since it requires a modern EGLDisplay and EGLSurface
    org.maplibre.android.maps.renderer.surfaceview.MapLibreGLSurfaceView surfaceView = new org.maplibre.android.maps.renderer.surfaceview.MapLibreGLSurfaceView(context);
    surfaceView.setZOrderMediaOverlay(renderSurfaceOnTop);

    return new org.maplibre.android.maps.renderer.surfaceview.GLSurfaceViewMapRenderer(context, surfaceView, localFontFamily, threadPriorityOverride) {
      @Override
      public void onSurfaceCreated(Surface surface) {
        initCallback.run();
        super.onSurfaceCreated(surface);
      }
    };
  }
}
