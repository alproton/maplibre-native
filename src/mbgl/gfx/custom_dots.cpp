
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

    if (!enabled || empty()) {
        return;
    }
    auto screenSize = transform.getSize();

    bool updateVertexBuffer = pointsChanged || transformChanged(transform);
    pointsChanged = false;
    if (updateVertexBuffer) {
        CustomDotsVertexBuffer vertexBuffer;
        vertexBuffer.reserve(pointCount() * 2);
        for (auto& p : data) {
            p.second.vertexOffset = static_cast<int>(vertexBuffer.size() / 2);
            p.second.vertexCount = 0;
            for (const auto& point : p.second.points) {
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
                ++p.second.vertexCount;
            }
        }
        updateVertexBufferImpl(vertexBuffer);
    }

    for (auto& p : data) {
        p.second.iconDx = p.second.options.outerRadius / static_cast<float>(screenSize.width);
        p.second.iconDy = p.second.options.outerRadius / static_cast<float>(screenSize.height);
        p.second.innerFactor = p.second.options.innerRadius > 0
                                   ? p.second.options.outerRadius / p.second.options.innerRadius
                                   : 0;
    }

    drawImpl();
}

void CustomDots::setPoints(int id, CustomDotsPoints points) {
    std::lock_guard<std::mutex> lock(mux);
    data[id].points = std::move(points);
    pointsChanged = true;
}

void CustomDots::setOptions(int id, const CustomDotsOptions& options) {
    std::lock_guard<std::mutex> lock(mux);
    data[id].options = options;
}

void CustomDots::setEnabled(bool enabled_) {
    std::lock_guard<std::mutex> lock(mux);
    enabled = enabled_;
}

void CustomDots::deallocateDotsMemory() {
    std::lock_guard<std::mutex> lock(mux);
    // This function is called in the UI thread and sets the clearVertexBuffer flag to
    // mark the vertex buffer for deletion. Actual deletion is done in the render thread
    clearVertexBuffer = true;
    for (auto& p : data) {
        p.second.points.clear();
        p.second.vertexOffset = 0;
        p.second.vertexCount = 0;
    }
}

void CustomDots::nextLayer(std::string next) {
    std::lock_guard<std::mutex> lock(mux);
    layer = std::move(next);
}

const std::string& CustomDots::nextLayer() const {
    std::lock_guard<std::mutex> lock(mux);
    return layer;
}

bool CustomDots::empty() const {
    for (const auto& p : data) {
        if (!p.second.points.empty()) {
            return false;
        }
    }
    return true;
}

size_t CustomDots::pointCount() const {
    size_t count = 0;
    for (const auto& p : data) {
        count += p.second.points.size();
    }
    return count;
}

bool CustomDots::transformChanged(const TransformState& transform) {
    // reference point to compare to
    // use an actual point instead of an arbitrary constant
    LatLng ref{};
    for (const auto& p : data) {
        if (!p.second.points.empty()) {
            ref = p.second.points[0];
            break;
        }
    }

    // check if transform has changed such as a dot moved at least one pixel
    auto currentPoint = transform.latLngToScreenCoordinate(ref);
    auto currentPointX = static_cast<int>(std::round(currentPoint.x));
    auto currentPointY = static_cast<int>(std::round(currentPoint.y));
    auto changed = currentPointX != previousPointX || currentPointY != previousPointY;
    previousPointX = currentPointX;
    previousPointY = currentPointY;

    return changed;
}

} // namespace gfx
} // namespace mbgl
