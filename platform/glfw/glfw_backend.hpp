#pragma once

#include "mbgl/gfx/custom_puck.hpp"

#include <mbgl/util/size.hpp>
#include <mbgl/gfx/backend.hpp>

namespace mbgl {
namespace gfx {
class RendererBackend;
} // namespace gfx
} // namespace mbgl

struct GLFWwindow;

class GLFWBackend {
public:
    explicit GLFWBackend() = default;
    GLFWBackend(const GLFWBackend&) = delete;
    GLFWBackend& operator=(const GLFWBackend&) = delete;
    virtual ~GLFWBackend() = default;
    virtual void setCustomPuckState([[maybe_unused]] double lat,
                                    [[maybe_unused]] double lon,
                                    [[maybe_unused]] double bearing) {};
    virtual void enableCustomPuck([[maybe_unused]] bool onOff) {};

    static std::unique_ptr<GLFWBackend> Create(GLFWwindow* window, bool capFrameRate) {
        return mbgl::gfx::Backend::Create<GLFWBackend, GLFWwindow*, bool>(window, capFrameRate);
    }

    virtual mbgl::gfx::RendererBackend& getRendererBackend() = 0;
    virtual mbgl::Size getSize() const = 0;
    virtual void setSize(mbgl::Size) = 0;
};
