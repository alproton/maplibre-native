#pragma once

#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/color.hpp>
#include <vector>
#include <mutex>

namespace mbgl {
namespace gfx {

struct CustomDotsOptions {
    std::string nextLayer;
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

    void setPoints(CustomDotsPoints points_);

    void clearVideoMemory();

    void setOptions(const CustomDotsOptions& options_);

    void setEnabled(bool enabled_);

    const std::string& nextLayer() const;

protected:
    using CustomDotsVertexBuffer = std::vector<float>;

    virtual void drawImpl() = 0;
    virtual void updateVertexBufferImpl(const CustomDotsVertexBuffer&) = 0;
    virtual void clearVertexBufferImpl() = 0;

protected:
    mutable std::mutex mux;
    int previousPointX = -1;
    int previousPointY = -1;
    CustomDotsPoints points;
    CustomDotsOptions options;
    float iconDx = 0;
    float iconDy = 0;
    float innerFactor = 0;
    bool pointsChanged = false;
    bool enabled = false;
    bool clearVertexBuffer = false;
};

} // namespace gfx
} // namespace mbgl
