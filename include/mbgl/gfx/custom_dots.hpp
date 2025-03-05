#pragma once

#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/color.hpp>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace mbgl {
namespace gfx {

struct CustomDotsOptions {
    Color innerColor = Color::black();
    Color outerColor = Color::black();
    float innerRadius = 0;
    float outerRadius = 0;
};

using CustomDotsPoints = std::vector<LatLng>;

class CustomDots {
public:
    virtual ~CustomDots() noexcept {};

    void draw(const TransformState& transform);

    void setPoints(int id, CustomDotsPoints points);

    void setOptions(int id, const CustomDotsOptions& options);

    // Enable the rendering of custom dots
    // Setting false will skip rendering without deallocating the memory used to store the dots
    void setEnabled(bool enabled_);

    // This function deallocates the memory used to store the dots
    void deallocateDotsMemory();

    void nextLayer(std::string next);

    const std::string& nextLayer() const;

protected:
    using CustomDotsVertexBuffer = std::vector<float>;

    struct CustomDotsData {
        CustomDotsPoints points;
        CustomDotsOptions options;
        float iconDx = 0;
        float iconDy = 0;
        float innerFactor = 0;
        int vertexOffset = 0;
        int vertexCount = 0;
    };

    virtual void drawImpl() = 0;
    virtual void updateVertexBufferImpl(const CustomDotsVertexBuffer&) = 0;
    virtual void clearVertexBufferImpl() = 0;

    bool transformChanged(const TransformState& transform);
    bool empty() const;
    size_t pointCount() const;

protected:
    mutable std::mutex mux; // ensure UI and render thread don't access the same data
    std::unordered_map<int, CustomDotsData> data;
    std::string layer = "";
    int previousPointX = -1;
    int previousPointY = -1;
    bool pointsChanged = false;
    bool enabled = false;
    bool clearVertexBuffer = false;
};

} // namespace gfx
} // namespace mbgl
