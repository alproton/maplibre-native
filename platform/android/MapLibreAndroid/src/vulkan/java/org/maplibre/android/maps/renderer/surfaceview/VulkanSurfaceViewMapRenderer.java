package org.maplibre.android.maps.renderer.surfaceview;

import android.content.Context;
import androidx.annotation.NonNull;

public class VulkanSurfaceViewMapRenderer extends SurfaceViewMapRenderer {

  public VulkanSurfaceViewMapRenderer(Context context,
                                @NonNull MapLibreVulkanSurfaceView surfaceView,
                                String localIdeographFontFamily, boolean multiThreadedGpuResourceUpload) {
    super(context, surfaceView, localIdeographFontFamily, multiThreadedGpuResourceUpload);

    this.surfaceView.setRenderer(this);
  }

}