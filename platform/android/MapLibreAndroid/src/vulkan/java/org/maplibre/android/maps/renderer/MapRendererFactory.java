package org.maplibre.android.maps.renderer;

import android.content.Context;
import android.view.Surface;
import android.view.TextureView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import org.maplibre.android.maps.renderer.surfaceview.MapLibreVulkanSurfaceView;
import org.maplibre.android.maps.renderer.surfaceview.SurfaceViewMapRenderer;
import org.maplibre.android.maps.renderer.surfaceview.VulkanSurfaceViewMapRenderer;
import org.maplibre.android.maps.renderer.textureview.TextureViewMapRenderer;
import org.maplibre.android.maps.renderer.textureview.VulkanTextureViewRenderThread;

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

    mapRenderer.setRenderThread(new VulkanTextureViewRenderThread(textureView, mapRenderer));
    return mapRenderer;
  }

  public static SurfaceViewMapRenderer newSurfaceViewMapRenderer(@NonNull Context context, String localFontFamily,
                                                                 boolean renderSurfaceOnTop, Runnable initCallback,
                                                                 int threadPriorityOverride, boolean useModernEGL,
                                                                 boolean useSwappy, boolean enableSwappyLogging) {
    // Note: Vulkan doesn't use the EGL/Swappy parameters, but we accept them for API compatibility

    MapLibreVulkanSurfaceView surfaceView = new MapLibreVulkanSurfaceView(context);
    surfaceView.setZOrderMediaOverlay(renderSurfaceOnTop);

    return new VulkanSurfaceViewMapRenderer(context, surfaceView, localFontFamily, threadPriorityOverride) {
      @Override
      public void onSurfaceCreated(Surface surface) {
        initCallback.run();
        super.onSurfaceCreated(surface);
      }
    };
  }
}
