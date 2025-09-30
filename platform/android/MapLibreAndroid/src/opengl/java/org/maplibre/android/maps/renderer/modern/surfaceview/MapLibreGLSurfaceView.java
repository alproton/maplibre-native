package org.maplibre.android.maps.renderer.modern.surfaceview;

import android.content.Context;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLSurface;
import android.opengl.GLSurfaceView;
import android.opengl.EGLDisplay;
import android.util.AttributeSet;
import android.util.Log;

import org.maplibre.android.log.Logger;
import org.maplibre.android.maps.renderer.SwappyPerformanceMonitor;
import org.maplibre.android.maps.renderer.SwappyRenderer;
import org.maplibre.android.maps.renderer.egl.EGLLogWrapper;
import org.maplibre.android.maps.renderer.modern.egl.EGLConfigChooser;
import org.maplibre.android.maps.renderer.modern.egl.EGLContextFactory;
import org.maplibre.android.maps.renderer.modern.egl.EGLWindowSurfaceFactory;
import org.maplibre.android.maps.renderer.surfaceview.MapLibreSurfaceView;
import org.maplibre.android.maps.renderer.surfaceview.SurfaceViewMapRenderer;

import java.lang.ref.WeakReference;

public class MapLibreGLSurfaceView extends MapLibreSurfaceView {

  protected final WeakReference<org.maplibre.android.maps.renderer.modern.surfaceview.MapLibreGLSurfaceView> viewWeakReference = new WeakReference<>(this);

  private EGLConfigChooser eglConfigChooser;
  private EGLContextFactory eglContextFactory;
  private EGLWindowSurfaceFactory eglWindowSurfaceFactory;
  private WeakReference<EGLSurface> eglSurfaceWeakReference;

  private boolean preserveEGLContextOnPause;
  private boolean useSwappy;
  private boolean enableSwappyLogging;

  // Timer for diagnostics - only call every 5 seconds
  private long lastDiagnosticsTime = 0;
  private static final long DIAGNOSTICS_INTERVAL_MS = 5000; // 5 seconds

  public MapLibreGLSurfaceView(Context context, boolean useSwappy, boolean enableSwappyLogging) {
    super(context);
    this.useSwappy = useSwappy;
    this.enableSwappyLogging = enableSwappyLogging;
  }

  public MapLibreGLSurfaceView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  /**
   * Control whether the EGL context is preserved when the GLSurfaceView is paused and
   * resumed.
   * <p>
   * If set to true, then the EGL context may be preserved when the GLSurfaceView is paused.
   * <p>
   * Prior to API level 11, whether the EGL context is actually preserved or not
   * depends upon whether the Android device can support an arbitrary number of
   * EGL contexts or not. Devices that can only support a limited number of EGL
   * contexts must release the EGL context in order to allow multiple applications
   * to share the GPU.
   * <p>
   * If set to false, the EGL context will be released when the GLSurfaceView is paused,
   * and recreated when the GLSurfaceView is resumed.
   * <p>
   * <p>
   * The default is false.
   *
   * @param preserveOnPause preserve the EGL context when paused
   */
  public void setPreserveEGLContextOnPause(boolean preserveOnPause) {
    preserveEGLContextOnPause = preserveOnPause;
  }

  /**
   * @return true if the EGL context will be preserved when paused
   */
  public boolean getPreserveEGLContextOnPause() {
    return preserveEGLContextOnPause;
  }

  public void setRenderer(SurfaceViewMapRenderer renderer) {
    if (eglConfigChooser == null) {
      throw new IllegalStateException("No eglConfigChooser provided");
    }
    if (eglContextFactory == null) {
      throw new IllegalStateException("No eglContextFactory provided");
    }
    if (eglWindowSurfaceFactory == null) {
      throw new IllegalStateException("No eglWindowSurfaceFactory provided");
    }

    super.setRenderer(renderer);
  }

  /**
   * Install a custom EGLContextFactory.
   * <p>If this method is
   * called, it must be called before {@link #setRenderer(SurfaceViewMapRenderer)}
   * is called.
   */
  public void setEGLContextFactory(EGLContextFactory factory) {
    checkRenderThreadState();
    eglContextFactory = factory;
  }

  /**
   * Install a custom EGLWindowSurfaceFactory.
   * <p>If this method is
   * called, it must be called before {@link #setRenderer(SurfaceViewMapRenderer)}
   * is called.
   */
  public void setEGLWindowSurfaceFactory(EGLWindowSurfaceFactory factory) {
    checkRenderThreadState();
    eglWindowSurfaceFactory = factory;
  }

  /**
   * Install a custom EGLConfigChooser.
   * <p>If this method is
   * called, it must be called before {@link #setRenderer(SurfaceViewMapRenderer)}
   * is called.
   */
  public void setEGLConfigChooser(EGLConfigChooser configChooser) {
    checkRenderThreadState();
    eglConfigChooser = configChooser;
  }

  @Override
  protected void createRenderThread() {
    renderThread = new GLThread(viewWeakReference);
  }

  public boolean recordFrameStart() {
    boolean success = false;
    if(SwappyRenderer.isEnabled() && eglSurfaceWeakReference != null && eglSurfaceWeakReference.get() != null) {
      EGLDisplay display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
      EGLSurface surface = eglSurfaceWeakReference.get();
      if(display != null && display != EGL14.EGL_NO_DISPLAY && surface != null && surface != EGL14.EGL_NO_SURFACE) {
        long displayHandle = display.getNativeHandle();
        long surfaceHandle = surface.getNativeHandle();
        SwappyPerformanceMonitor.recordFrameStart(displayHandle, surfaceHandle);

        // Only call emergency diagnostics every 5 seconds instead of every frame
        long currentTime = System.currentTimeMillis();
        if (currentTime - lastDiagnosticsTime >= DIAGNOSTICS_INTERVAL_MS) {
          SwappyPerformanceMonitor.emergencyDiagnostics();
          lastDiagnosticsTime = currentTime;
        }
      }

      success = true;
    }

    return success;
  }

  /**
   * An EGL helper class.
   */
  private static class EglHelper {
    private EglHelper(WeakReference<MapLibreGLSurfaceView> glSurfaceViewWeakRef) {
      mGLSurfaceViewWeakRef = glSurfaceViewWeakRef;
    }

    /**
     * Initialize EGL for a given configuration spec.
     */
    public void start() {
      try {

        /*
         * Get to the default display.
         */
        mEglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);

        if (mEglDisplay == EGL14.EGL_NO_DISPLAY) {
          Log.e(TAG, "eglGetDisplay failed");
          return;
        }

        /*
         * We can now initialize EGL for that display
         */
        int[] major = new int[1];
        int[] minor = new int[1];
        if (!EGL14.eglInitialize(mEglDisplay, major, 0, minor, 0)) {
          Log.e(TAG, "eglInitialize failed");
          return;
        }
        MapLibreGLSurfaceView view = mGLSurfaceViewWeakRef.get();
        if (view == null) {
          mEglConfig = null;
          mEglContext = null;
        } else {
          mEglConfig = view.eglConfigChooser.chooseConfig(mEglDisplay);
          if (mEglConfig == null) {
            Log.e(TAG, "failed to select an EGL configuration");
            return;
          }

          /*
           * Create an EGL context. We want to do this as rarely as we can, because an
           * EGL context is a somewhat heavy object.
           */
          mEglContext = view.eglContextFactory.createContext(mEglDisplay, mEglConfig);
        }
        if (mEglContext == null || mEglContext == EGL14.EGL_NO_CONTEXT) {
          mEglContext = null;
          Log.e(TAG, "createContext failed");
          return;
        }
      } catch (Exception exception) {
        Log.e(TAG, "createContext failed: ", exception);
      }
      mEglSurface = null;
    }

    /**
     * Create an egl surface for the current SurfaceHolder surface. If a surface
     * already exists, destroy it before creating the new surface.
     *
     * @return true if the surface was created successfully.
     */
    boolean createSurface() {
      if (mEglDisplay == null) {
        Log.e(TAG, "eglDisplay not initialized");
        return false;
      }
      if (mEglConfig == null) {
        Log.e(TAG, "mEglConfig not initialized");
        return false;
      }

      /*
       *  The window size has changed, so we need to create a new
       *  surface.
       */
      destroySurfaceImp();

      /*
       * Create an EGL surface we can render into.
       */
      MapLibreGLSurfaceView view = mGLSurfaceViewWeakRef.get();
      if (view != null) {
        final int[] surfaceAttributes = {
                EGL14.EGL_NONE
        };

        mEglSurface = view.eglWindowSurfaceFactory.createWindowSurface(mEglDisplay, mEglConfig, view.getHolder(), surfaceAttributes, 0);
        view.eglSurfaceWeakReference = new WeakReference<>(mEglSurface);
      } else {
        mEglSurface = null;
      }

      if (mEglSurface == null || mEglSurface == EGL14.EGL_NO_SURFACE) {
        int error = EGL14.eglGetError();
        if (error == EGL14.EGL_BAD_NATIVE_WINDOW) {
          Log.e(TAG, "createWindowSurface returned EGL_BAD_NATIVE_WINDOW.");
        }
        return false;
      }

      /*
       * Before we can issue GL commands, we need to make sure
       * the context is current and bound to a surface.
       */
      if (!EGL14.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
        /*
         * Could not make the context current, probably because the underlying
         * SurfaceView surface has been destroyed.
         */
        logEglErrorAsWarning(TAG, "eglMakeCurrent", EGL14.eglGetError());
        return false;
      }

      return true;
    }

    /**
     * Display the current render surface.
     *
     * @return the EGL error code from eglSwapBuffers.
     */
    public int swap() {
      if(SwappyRenderer.isEnabled()) {
        boolean success = SwappyRenderer.swap(mEglDisplay.getNativeHandle(), mEglSurface.getNativeHandle());

        return success ? EGL14.EGL_SUCCESS : EGL14.eglGetError();
      }

      if (!EGL14.eglSwapBuffers(mEglDisplay, mEglSurface)) {
        return EGL14.eglGetError();
      }
      return EGL14.EGL_SUCCESS;
    }

    void destroySurface() {
      destroySurfaceImp();
    }

    private void destroySurfaceImp() {
      if (mEglSurface != null && mEglSurface != EGL14.EGL_NO_SURFACE) {
        EGL14.eglMakeCurrent(mEglDisplay, EGL14.EGL_NO_SURFACE,
                EGL14.EGL_NO_SURFACE,
                EGL14.EGL_NO_CONTEXT);
        MapLibreGLSurfaceView view = mGLSurfaceViewWeakRef.get();
        if (view != null) {
          view.eglWindowSurfaceFactory.destroySurface(mEglDisplay, mEglSurface);
        }
        mEglSurface = null;
      }
    }

    public void finish() {
      if (mEglContext != null) {
        MapLibreGLSurfaceView view = mGLSurfaceViewWeakRef.get();
        if (view != null) {
          view.eglContextFactory.destroyContext(mEglDisplay, mEglContext);
        }
        mEglContext = null;
      }
      if (mEglDisplay != null) {
        EGL14.eglTerminate(mEglDisplay);
        mEglDisplay = null;
      }
    }

    static void logEglErrorAsWarning(String tag, String function, int error) {
      Log.w(tag, formatEglError(function, error));
    }

    static String formatEglError(String function, int error) {
      return function + " failed: " + EGLLogWrapper.getErrorString(error);
    }

    private WeakReference<MapLibreGLSurfaceView> mGLSurfaceViewWeakRef;

    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLConfig mEglConfig;
    EGLContext mEglContext;

  }

  /**
   * A generic GL Thread. Takes care of initializing EGL and GL. Delegates
   * to a Renderer instance to do the actual drawing. Can be configured to
   * render continuously or on request.
   * <p>
   * All potentially blocking synchronization is done through the
   * sGLThreadManager object. This avoids multiple-lock ordering issues.
   */
  static class GLThread extends MapLibreSurfaceView.RenderThread {
    GLThread(WeakReference<MapLibreGLSurfaceView> surfaceViewWeakRef) {
      super(surfaceViewWeakRef.get().renderThreadManager);

      mSurfaceViewWeakRef = surfaceViewWeakRef;
    }

    /*
     * This private method should only be called inside a
     * synchronized(sGLThreadManager) block.
     */
    private void stopEglSurfaceLocked() {
      if (haveEglSurface) {
        haveEglSurface = false;
        eglHelper.destroySurface();
      }
    }

    /*
     * This private method should only be called inside a
     * synchronized(sGLThreadManager) block.
     */
    private void stopEglContextLocked() {
      if (haveEglContext) {
        eglHelper.finish();
        haveEglContext = false;
        renderThreadManager.notifyAll();
      }
    }

    @Override
    protected void guardedRun() throws InterruptedException {
      eglHelper = new EglHelper(mSurfaceViewWeakRef);
      haveEglContext = false;
      haveEglSurface = false;
      wantRenderNotification = false;

      try {
        boolean createEglContext = false;
        boolean createEglSurface = false;
        boolean lostEglContext = false;
        boolean sizeChanged = false;
        boolean wantRenderNotification = false;
        boolean doRenderNotification = false;
        boolean askedToReleaseEglContext = false;
        boolean isWaitingFrame = false;
        int w = 0;
        int h = 0;
        Runnable event = null;
        Runnable finishDrawingRunnable = null;

        while (true) {
          synchronized (renderThreadManager) {
            while (true) {
              if (shouldExit) {
                return;
              }

              if (!eventQueue.isEmpty()) {
                event = eventQueue.remove(0);
                break;
              }

              // Update the pause state.
              boolean pausing = false;
              if (paused != requestPaused) {
                pausing = requestPaused;
                paused = requestPaused;
                renderThreadManager.notifyAll();
              }

              // Do we need to give up the EGL context?
              if (shouldReleaseEglContext) {
                stopEglSurfaceLocked();
                stopEglContextLocked();
                shouldReleaseEglContext = false;
                askedToReleaseEglContext = true;
              }

              // Have we lost the EGL context?
              if (lostEglContext) {
                stopEglSurfaceLocked();
                stopEglContextLocked();
                lostEglContext = false;
              }

              // When pausing, release the EGL surface:
              if (pausing && haveEglSurface) {
                stopEglSurfaceLocked();
              }

              // When pausing, optionally release the EGL Context:
              if (pausing && haveEglContext) {
                MapLibreGLSurfaceView view = mSurfaceViewWeakRef.get();
                boolean preserveEglContextOnPause = view != null && view.preserveEGLContextOnPause;
                if (!preserveEglContextOnPause) {
                  stopEglContextLocked();
                }
              }

              // Have we lost the SurfaceView surface?
              if ((!hasSurface) && (!waitingForSurface)) {
                if (haveEglSurface) {
                  stopEglSurfaceLocked();
                }
                waitingForSurface = true;
                surfaceIsBad = false;
                renderThreadManager.notifyAll();
              }

              // Have we acquired the surface view surface?
              if (hasSurface && waitingForSurface) {
                waitingForSurface = false;
                renderThreadManager.notifyAll();
              }

              if (doRenderNotification) {
                this.wantRenderNotification = false;
                doRenderNotification = false;
                renderComplete = true;
                renderThreadManager.notifyAll();
              }

              if (this.finishDrawingRunnable != null) {
                finishDrawingRunnable = this.finishDrawingRunnable;
                this.finishDrawingRunnable = null;
              }

              // Ready to draw?
              if (readyToDraw()) {

                // If we don't have an EGL context, try to acquire one.
                if (!haveEglContext) {
                  if (askedToReleaseEglContext) {
                    askedToReleaseEglContext = false;
                  } else {
                    try {
                      eglHelper.start();
                    } catch (RuntimeException exception) {
                      renderThreadManager.notifyAll();
                      return;
                    }
                    haveEglContext = true;
                    createEglContext = true;

                    renderThreadManager.notifyAll();
                  }
                }

                if (haveEglContext && !haveEglSurface) {
                  haveEglSurface = true;
                  createEglSurface = true;
                  sizeChanged = true;
                }

                if (haveEglSurface) {
                  if (this.sizeChanged) {
                    sizeChanged = true;
                    w = width;
                    h = height;
                    this.wantRenderNotification = true;

                    // Destroy and recreate the EGL surface.
                    createEglSurface = true;

                    this.sizeChanged = false;
                  }
                  requestRender = false;
                  renderThreadManager.notifyAll();
                  if (this.wantRenderNotification) {
                    wantRenderNotification = true;
                  }
                  break;
                }
              } else {
                if (finishDrawingRunnable != null) {
                  Log.w(TAG, "Warning, !readyToDraw() but waiting for "
                          + "draw finished! Early reporting draw finished.");
                  finishDrawingRunnable.run();
                  finishDrawingRunnable = null;
                }
              }
              // By design, this is the only place in a GLThread thread where we wait().
              isWaitingFrame = true;
              renderThreadManager.wait();
            }
          } // end of synchronized(sGLThreadManager)

          if (event != null) {
            event.run();
            event = null;
            continue;
          }

          if (createEglSurface) {
            if (eglHelper.createSurface()) {
              synchronized (renderThreadManager) {
                finishedCreatingEglSurface = true;
                renderThreadManager.notifyAll();
              }
            } else {
              synchronized (renderThreadManager) {
                finishedCreatingEglSurface = true;
                surfaceIsBad = true;
                renderThreadManager.notifyAll();
              }
              continue;
            }
            createEglSurface = false;
          }

          if (createEglContext) {
            MapLibreGLSurfaceView view = mSurfaceViewWeakRef.get();
            if (view != null) {
              view.renderer.onSurfaceCreated(null);
            }
            createEglContext = false;
          }

          if (sizeChanged) {
            MapLibreGLSurfaceView view = mSurfaceViewWeakRef.get();
            if (view != null) {
              view.renderer.onSurfaceChanged(w, h);
            }
            sizeChanged = false;
          }

          MapLibreGLSurfaceView view = mSurfaceViewWeakRef.get();
          if (view != null) {
            view.renderer.onDrawFrame(isWaitingFrame);
            isWaitingFrame = false;
            if (finishDrawingRunnable != null) {
              finishDrawingRunnable.run();
              finishDrawingRunnable = null;
            }
          }
          int swapError = eglHelper.swap();
          switch (swapError) {
            case EGL14.EGL_SUCCESS:
              break;
            case EGL14.EGL_CONTEXT_LOST:
              lostEglContext = true;
              break;
            default:
              // Other errors typically mean that the current surface is bad,
              // probably because the SurfaceView surface has been destroyed,
              // but we haven't been notified yet.
              // Log the error to help developers understand why rendering stopped.
              EglHelper.logEglErrorAsWarning(TAG, "eglSwapBuffers", swapError);

              synchronized (renderThreadManager) {
                surfaceIsBad = true;
                renderThreadManager.notifyAll();
              }
              break;
          }

          if (wantRenderNotification) {
            doRenderNotification = true;
            wantRenderNotification = false;
          }
        }
      }
         finally {
        /*
         * clean-up everything...
         */
        synchronized (renderThreadManager) {
          stopEglSurfaceLocked();
          stopEglContextLocked();
        }
      }
    }

    @Override
    protected boolean readyToDraw() {
      return super.readyToDraw() && !surfaceIsBad;
    }

    @Override
    public boolean ableToDraw() {
      return haveEglContext && haveEglSurface && readyToDraw();
    }

    public void surfaceCreated() {
      synchronized (renderThreadManager) {
        hasSurface = true;
        finishedCreatingEglSurface = false;
        renderThreadManager.notifyAll();
        while (waitingForSurface
          && !finishedCreatingEglSurface
          && !exited) {
          try {
            renderThreadManager.wait();
          } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
          }
        }
      }
    }

    public void requestReleaseEglContextLocked() {
      shouldReleaseEglContext = true;
      renderThreadManager.notifyAll();
    }

    private boolean surfaceIsBad;
    private boolean haveEglContext;
    private boolean haveEglSurface;
    private boolean finishedCreatingEglSurface;
    private boolean shouldReleaseEglContext;

    private EglHelper eglHelper;

    /**
     * Set once at thread construction time, nulled out when the parent view is garbage
     * called. This weak reference allows the SurfaceView to be garbage collected while
     * the RenderThread is still alive.
     */
    protected WeakReference<org.maplibre.android.maps.renderer.modern.surfaceview.MapLibreGLSurfaceView> mSurfaceViewWeakRef;
  }
}
