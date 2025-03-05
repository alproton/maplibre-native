
#include <mbgl/gfx/custom_dots.hpp>
#include <numbers>
#include <mutex>

namespace mbgl {
namespace gfx {

namespace {} // namespace

void CustomDots::draw(const TransformState& transform) {
    std::lock_guard<std::mutex> lock(mux);

    if (!enabled || points.empty()) {
        return;
    }
    auto screenSize = transform.getSize();

    bool updateVertexBuffer = pointsChanged || transform.getProjectionMatrix() != previousTransform;
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
        if (vertexBuffer.empty()) {
            return;
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

} // namespace gfx
} // namespace mbgl
