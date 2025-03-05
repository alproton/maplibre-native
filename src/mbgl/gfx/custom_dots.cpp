
#include <mbgl/gfx/custom_dots.hpp>
#include <numbers>
#include <mutex>

namespace mbgl {
namespace gfx {

void CustomDots::draw(const TransformState& transform) {
    std::lock_guard<std::mutex> lock(mux);

    if (clearVertexBuffer) {
        clearVertexBufferImpl();
        clearVertexBuffer = false;
    }

    if (!enabled || points.empty()) {
        return;
    }
    auto screenSize = transform.getSize();

    // check if transform has changed such as a dot moved at least one pixel
    auto currentPoint = transform.latLngToScreenCoordinate(points[0]);
    auto currentPointX = static_cast<int>(std::round(currentPoint.x));
    auto currentPointY = static_cast<int>(std::round(currentPoint.y));
    auto transformChanged = currentPointX != previousPointX || currentPointY != previousPointY;
    previousPointX = currentPointX;
    previousPointY = currentPointY;

    bool updateVertexBuffer = pointsChanged || transformChanged;
    pointsChanged = false;
    if (updateVertexBuffer) {
        CustomDotsVertexBuffer vertexBuffer;
        vertexBuffer.reserve(points.size() * 2);
        for (const auto& point : points) {
            auto screenCoord = transform.latLngToScreenCoordinate(point);
            auto x = screenCoord.x / screenSize.width;
            auto y = screenCoord.y / screenSize.height;
            x = x * 2 - 1;
            y = y * 2 - 1;
            if (std::abs(x) > 1 || std::abs(y) > 1) {
                continue;
            }
            vertexBuffer.push_back(static_cast<float>(x));
            vertexBuffer.push_back(static_cast<float>(y));
        }
        updateVertexBufferImpl(vertexBuffer);
    }

    iconDx = options.outerRadius / static_cast<float>(screenSize.width);
    iconDy = options.outerRadius / static_cast<float>(screenSize.height);
    if (options.innerRadius <= 0) {
        options.innerRadius = 1;
    }
    innerFactor = options.outerRadius / options.innerRadius;

    drawImpl();
}

void CustomDots::setPoints(CustomDotsPoints points_) {
    std::lock_guard<std::mutex> lock(mux);
    points = std::move(points_);
    pointsChanged = true;
}

void CustomDots::setOptions(const CustomDotsOptions& options_) {
    std::lock_guard<std::mutex> lock(mux);
    options = options_;
}

void CustomDots::setEnabled(bool enabled_) {
    std::lock_guard<std::mutex> lock(mux);
    enabled = enabled_;
}

void CustomDots::clearVideoMemory() {
    std::lock_guard<std::mutex> lock(mux);
    clearVertexBuffer = true;
    points.clear();
}

const std::string& CustomDots::nextLayer() const {
    std::lock_guard<std::mutex> lock(mux);
    return options.nextLayer;
}

} // namespace gfx
} // namespace mbgl
