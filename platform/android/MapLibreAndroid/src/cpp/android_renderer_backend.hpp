#pragma once

#include <mbgl/gfx/backend.hpp>
#include <mbgl/gfx/custom_puck.hpp>
#include <mbgl/gfx/custom_dots.hpp>
#include <mbgl/gfx/renderer_backend.hpp>
#include <mbgl/gfx/renderable.hpp>

#include <android/native_window.h>

namespace mbgl {
namespace android {

class AndroidRendererBackend {
public:
    AndroidRendererBackend() = default;
    AndroidRendererBackend(const AndroidRendererBackend&) = delete;
    AndroidRendererBackend& operator=(const AndroidRendererBackend&) = delete;
    virtual ~AndroidRendererBackend() = default;

    static std::unique_ptr<AndroidRendererBackend> Create(ANativeWindow* window) {
        return mbgl::gfx::Backend::Create<AndroidRendererBackend, ANativeWindow*>(window);
    }
    virtual mbgl::gfx::RendererBackend& getImpl() = 0;

    virtual void updateViewPort();

    // Ensures the current context is not cleaned up when destroyed
    virtual void markContextLost();

    virtual void resizeFramebuffer(int width, int height);
    virtual PremultipliedImage readFramebuffer();

    gfx::Renderable::SwapBehaviour getSwapBehavior() const { return swapBehaviour; }
    virtual void setSwapBehavior(gfx::Renderable::SwapBehaviour swapBehaviour);

    virtual void setSwapInterval(int interval);

    int getSwapInterval() const { return swapInterval; }

    void setPuckStyle(const std::string& style_file_path);

    void setPuckAssetManager(AAssetManager* asset_manager);

    void setPuckVariant(const std::string& variant);

    void setPuckIconState(const std::string& state, const std::string& secondaryState = "");

    void setCustomPuckState(const gfx::CustomPuckState& state) noexcept { customPuckState = state; }

    const gfx::CustomPuckState& getCustomPuckState() const noexcept { return customPuckState; }

    void setCustomDotsNextLayer(std::string layer);

    void setCustomDotsPoints(int id, gfx::CustomDotsPoints points);

    void clearCustomDotsVideoMemory();

    void setCustomDotsOptions(int id, const gfx::CustomDotsOptions& options);

    void setCustomDotsEnabled(bool enabled);

    bool isCustomDotsInitialized();

protected:
    gfx::Renderable::SwapBehaviour swapBehaviour = gfx::Renderable::SwapBehaviour::NoFlush;
    int swapInterval = -1;
    gfx::CustomPuckState customPuckState;
};

} // namespace android
} // namespace mbgl
