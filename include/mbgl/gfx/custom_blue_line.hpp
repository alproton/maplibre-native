#pragma once

#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/geometry.hpp>
#include <mutex>
#include <vector>

namespace mbgl {
namespace gfx {

class CustomBlueLine {
public:
    virtual ~CustomBlueLine() noexcept {};

    void draw(const TransformState& transform);

    void clearCustomBlueLine();
    void setCustomBlueLine(LineString<double> line_);
    void setCustomBlueLinePercent(double percent_);

protected:
    virtual void drawImpl() = 0;
    virtual void updateVertexBufferImpl(const std::vector<float>& vertices) = 0;
    virtual void clearVertexBufferImpl() = 0;

    bool transformChanged(const TransformState& transform);

protected:
    mutable std::mutex mux; // ensure UI and render thread don't access the same data
    double percent = 0;
    LineString<double> line;
    bool clearVertexBuffer = false;

    double previousPercent = -1;
    int previousPointX = -1;
    int previousPointY = -1;
};

} // namespace gfx
} // namespace mbgl
