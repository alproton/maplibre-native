#pragma once

#include <mbgl/gfx/custom_puck_style.hpp>
#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/image.hpp>
#include <array>
#include <chrono>
#include <memory>
#include <string>

namespace mbgl {
namespace gfx {

using CustomPuckScreenQuad = std::array<ScreenCoordinate, 4>;

struct CustomPuckState {
    double lat = 0;
    double lon = 0;
    double bearing = 0;
    float iconScale = 1.f;
    bool cameraTracking = false;
    bool enabled = false;
};

struct CustomPuckSampledIcon {
    std::string name;
    CustomPuckScreenQuad quad;
    Color color = Color::white();
};

struct CustomPuckSampledStyle {
    CustomPuckSampledIcon icon1;
    CustomPuckSampledIcon icon2;
    CustomPuckIconMap icons;
};

class CustomPuck {
public:
    virtual ~CustomPuck() noexcept {};

    void draw(const TransformState& transform);

    void setPuckStyle(const std::string& style_file_path);
    void setPuckVariant(std::string variant);
    void setPuckIconState(std::string state);

protected:
    virtual void drawImpl(const CustomPuckSampledStyle&) = 0;

    virtual CustomPuckState getState() = 0;

private:
    std::chrono::time_point<std::chrono::steady_clock> epochTime = std::chrono::steady_clock::now();
    PremultipliedImage bitmap{};
    CustomPuckStyle style{};
    std::string variantName;
    std::string iconStateName;
    // The style is updated in the UI thread and is used in the rendering thread
    // This mutex ensures the puck is not modified by the UI thread while being rendered in the render thread
    std::mutex styleMutex;
};

} // namespace gfx
} // namespace mbgl
