package org.maplibre.android.maps.renderer.modern.textureview;

import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.view.TextureView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;

import org.maplibre.android.log.Logger;
import org.maplibre.android.maps.renderer.modern.egl.EGLConfigChooser;
import org.maplibre.android.maps.renderer.textureview.TextureViewMapRenderer;
import org.maplibre.android.maps.renderer.textureview.TextureViewRenderThread;

import java.lang.ref.WeakReference;

/**
 * The render thread is responsible for managing the communication between the
 * ui thread and the render thread it creates. Also, the EGL and GL contexts
 * are managed from here.
 */
public class GLTextureViewRenderThread extends TextureViewRenderThread {

  @NonNull
  private final EGLHolder eglHolder;

  private boolean destroyContext;

  /**
   * Create a render thread for the given TextureView / Maprenderer combination.
   *
   * @param textureView the TextureView
   * @param mapRenderer the MapRenderer
   */
  @UiThread
  public GLTextureViewRenderThread(@NonNull TextureView textureView, @NonNull TextureViewMapRenderer mapRenderer) {
    super(textureView, mapRenderer);
    this.eglHolder = new EGLHolder(new WeakReference<>(textureView), mapRenderer.isTranslucentSurface());
  }

  // Thread implementation

  @Override
  public void run() {
    try {

      while (true) {
        Runnable event = null;
        boolean initializeEGL = false;
        boolean recreateSurface = false;
        int w = -1;
        int h = -1;

        // Guarded block
        synchronized (lock) {
          while (true) {

            if (shouldExit) {
              return;
            }

            // If any events are scheduled, pop one for processing
            if (!eventQueue.isEmpty()) {
              event = eventQueue.remove(0);
              break;
            }

            if (destroySurface) {
              eglHolder.destroySurface();
              destroySurface = false;
              this.hasNativeSurface = false;
              lock.notifyAll();
              break;
            }

            if (destroyContext) {
              eglHolder.destroyContext();
              destroyContext = false;
              break;
            }

            if (surfaceTexture != null && !paused && requestRender) {

              w = width;
              h = height;

              // Initialize EGL if needed
              if (eglHolder.eglContext == EGL14.EGL_NO_CONTEXT) {
                this.hasNativeSurface = true;
                initializeEGL = true;
                break;
              }

              // (re-)Initialize EGL Surface if needed
              if (eglHolder.eglSurface == EGL14.EGL_NO_SURFACE) {
                this.hasNativeSurface = true;
                recreateSurface = true;
                break;
              }

              // Reset the request render flag now, so we can catch new requests
              // while rendering
              requestRender = false;

              // Break the guarded loop and continue to process
              break;
            }


            // Wait until needed
            lock.wait();

          } // end guarded while loop

        } // end guarded block

        // Run event, if any
        if (event != null) {
          event.run();
          continue;
        }

        // Initialize EGL
        if (initializeEGL) {
          eglHolder.prepare();
          synchronized (lock) {
            if (!eglHolder.createSurface()) {
              // Cleanup the surface if one could not be created
              // and wait for another to be ready.
              destroySurface = true;
              continue;
            }
          }
          mapRenderer.onSurfaceCreated(null);
          mapRenderer.onSurfaceChanged(w, h);
          continue;
        }

        // If the surface size has changed inform the map renderer.
        if (recreateSurface) {
          synchronized (lock) {
            eglHolder.createSurface();
          }
          mapRenderer.onSurfaceChanged(w, h);
          continue;
        }

        if (sizeChanged) {
          mapRenderer.onSurfaceChanged(w, h);
          sizeChanged = false;
          continue;
        }

        // Don't continue without a surface
        if (eglHolder.eglSurface == EGL14.EGL_NO_SURFACE) {
          continue;
        }

        // Time to render a frame
        mapRenderer.onDrawFrame();

        // Swap and check the result
        int swapError = eglHolder.swap();
        switch (swapError) {
          case EGL14.EGL_SUCCESS:
            break;
          case EGL14.EGL_CONTEXT_LOST:
            Logger.w(TAG, "Context lost. Waiting for re-aquire");
            synchronized (lock) {
              surfaceTexture = null;
              destroySurface = true;
              destroyContext = true;
            }
            break;
          default:
            Logger.w(TAG, String.format("eglSwapBuffer error: %s. Waiting or new surface", swapError));
            // Probably lost the surface. Clear the current one and
            // wait for a new one to be set
            synchronized (lock) {
              surfaceTexture = null;
              destroySurface = true;
            }
        }

      }

    } catch (InterruptedException err) {
      // To be expected
    } finally {
      // Cleanup
      eglHolder.cleanup();

      // Signal we're done
      synchronized (lock) {
        this.hasNativeSurface = false;
        this.exited = true;
        lock.notifyAll();
      }
    }
  }

  /**
   * Holds the EGL state and offers methods to mutate it.
   */
  private static class EGLHolder {
    private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    private final WeakReference<TextureView> textureViewWeakRef;
    private boolean translucentSurface;

    @Nullable
    private EGLConfig eglConfig;
    private EGLDisplay eglDisplay = EGL14.EGL_NO_DISPLAY;
    private EGLContext eglContext = EGL14.EGL_NO_CONTEXT;
    private EGLSurface eglSurface = EGL14.EGL_NO_SURFACE;

    EGLHolder(WeakReference<TextureView> textureViewWeakRef, boolean translucentSurface) {
      this.textureViewWeakRef = textureViewWeakRef;
      this.translucentSurface = translucentSurface;
    }

    void prepare() {

      // Only re-initialize display when needed
      if (eglDisplay == EGL14.EGL_NO_DISPLAY) {
        this.eglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);

        if (eglDisplay == EGL14.EGL_NO_DISPLAY) {
          throw new RuntimeException("eglGetDisplay failed");
        }

        int[] major = new int[1];
        int[] minor = new int[1];
        if (!EGL14.eglInitialize(eglDisplay, major, 0, minor, 0)) {
          throw new RuntimeException("eglInitialize failed");
        }
      }

      if (textureViewWeakRef == null) {
        // No texture view present
        eglConfig = null;
        eglContext = EGL14.EGL_NO_CONTEXT;
      } else if (eglContext == EGL14.EGL_NO_CONTEXT) {
        eglConfig = new EGLConfigChooser(translucentSurface).chooseConfig(eglDisplay);
        int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE};
        eglContext = EGL14.eglCreateContext(eglDisplay, eglConfig, EGL14.EGL_NO_CONTEXT, attrib_list, 0);
      }

      if (eglContext == EGL14.EGL_NO_CONTEXT) {
        throw new RuntimeException("createContext");
      }
    }

    @NonNull

    boolean createSurface() {
      // The window size has changed, so we need to create a new surface.
      destroySurface();

      // Create an EGL surface we can render into.
      TextureView view = textureViewWeakRef.get();
      if (view != null && view.getSurfaceTexture() != null) {
        int[] surfaceAttribs = {EGL14.EGL_NONE};
        eglSurface = EGL14.eglCreateWindowSurface(eglDisplay, eglConfig, view.getSurfaceTexture(), surfaceAttribs, 0);
      } else {
        eglSurface = EGL14.EGL_NO_SURFACE;
      }

      if (eglSurface == null || eglSurface == EGL14.EGL_NO_SURFACE) {
        int error = EGL14.eglGetError();
        if (error == EGL14.EGL_BAD_NATIVE_WINDOW) {
          Logger.e(TAG, "createWindowSurface returned EGL_BAD_NATIVE_WINDOW.");
        }
        return false;
      }

      return makeCurrent();
    }

    boolean makeCurrent() {
      if (!EGL14.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
        // Could not make the context current, probably because the underlying
        // SurfaceView surface has been destroyed.
        Logger.w(TAG, String.format("eglMakeCurrent: %s", EGL14.eglGetError()));
        return false;
      }

      return true;
    }

    int swap() {
      if (!EGL14.eglSwapBuffers(eglDisplay, eglSurface)) {
        return EGL14.eglGetError();
      }
      return EGL14.EGL_SUCCESS;
    }

    private void destroySurface() {
      if (eglSurface == EGL14.EGL_NO_SURFACE) {
        return;
      }

      if (!EGL14.eglDestroySurface(eglDisplay, eglSurface)) {
        Logger.w(TAG, String.format("Could not destroy egl surface. Display %s, Surface %s", eglDisplay, eglSurface));
      }

      eglSurface = EGL14.EGL_NO_SURFACE;
    }

    private void destroyContext() {
      if (eglContext == EGL14.EGL_NO_CONTEXT) {
        return;
      }

      if (!EGL14.eglDestroyContext(eglDisplay, eglContext)) {
        Logger.w(TAG, String.format("Could not destroy egl context. Display %s, Context %s", eglDisplay, eglContext));
      }

      eglContext = EGL14.EGL_NO_CONTEXT;
    }

    private void terminate() {
      if (eglDisplay == EGL14.EGL_NO_DISPLAY) {
        return;
      }

      if (!EGL14.eglTerminate(eglDisplay)) {
        Logger.w(TAG, String.format("Could not terminate egl. Display %s", eglDisplay));
      }
      eglDisplay = EGL14.EGL_NO_DISPLAY;
    }

    void cleanup() {
      destroySurface();
      destroyContext();
      terminate();
    }
  }
}
