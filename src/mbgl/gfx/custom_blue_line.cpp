
#include <mbgl/gfx/custom_blue_line.hpp>
#include <deque>
#include <algorithm>

namespace mbgl {
namespace gfx {

namespace {

struct Vertex {
    double x;
    double y;
    double distance;
};

} // namespace

void CustomBlueLine::draw(const TransformState& transform) {
    std::lock_guard<std::mutex> lock(mux);

    if (clearVertexBuffer) {
        clearVertexBufferImpl();
        clearVertexBuffer = false;
    }

    if (line.empty()) {
        return;
    }
    auto screenSize = transform.getSize();

    bool updateVertexBuffer = transformChanged(transform);
    if (updateVertexBuffer) {
        std::deque<Vertex> vertices;
        for (size_t i = 0; i < line.size(); ++i) {
            auto& p = line[i];
            auto screenCoord = transform.latLngToScreenCoordinate(LatLng(p.y, p.x));
            auto x = screenCoord.x / screenSize.width;
            auto y = screenCoord.y / screenSize.height;
            x = x * 2 - 1;
            y = y * 2 - 1;
            vertices.push_back({x, y, 0.0});
        }

        // Screen space distance per vertex
        double totalDistance = 0.0;
        for (size_t i = 1; i < vertices.size(); ++i) {
            auto dx = vertices[i].x - vertices[i - 1].x;
            auto dy = vertices[i].y - vertices[i - 1].y;
            totalDistance += std::sqrt(dx * dx + dy * dy);
            vertices[i].distance = totalDistance;
        }
        double vanishDistance = totalDistance * percent;

        // Clip vanished points
        if (vanishDistance > 0) {
            int firstVisibile = 0;
            while (firstVisibile < static_cast<int>(vertices.size()) &&
                   vertices[firstVisibile].distance < vanishDistance) {
                ++firstVisibile;
            }
            if (firstVisibile >= static_cast<int>(vertices.size())) {
                // all points are vanished
                clearVertexBufferImpl();
                return;
            }
            if (firstVisibile > 0) {
                --firstVisibile;
                for (int i = 0; i < firstVisibile; ++i) {
                    vertices.pop_front();
                }
            }
            // Replace the first vertex with the vertex that correspond to the vanish point
            assert(vertices[0].distance < vanishDistance);
            assert(vertices[1].distance >= vanishDistance);
            float t = (vanishDistance - vertices[0].distance) / (vertices[1].distance - vertices[0].distance);
            // Interpolate the first vertex to the vanish point
            vertices[0].x = vertices[0].x + t * (vertices[1].x - vertices[0].x);
            vertices[0].y = vertices[0].y + t * (vertices[1].y - vertices[0].y);
        }

        // simple clipping to screen bounds
        // The line can get in and out of the screen but we only clip the ends for simplicity
        // Also assumes one segment is fully on the screen or the line disappears (segment/quad clipping is needed)
        int start = 0;
        while (start < static_cast<int>(vertices.size()) &&
               (std::abs(vertices[start].x) > 1 || std::abs(vertices[start].y) > 1)) {
            ++start;
        }
        if (start >= static_cast<int>(vertices.size())) {
            // all points are outside the screen bounds
            clearVertexBufferImpl();
            return;
        }
        if (start > 0) {
            --start;
            for (int i = 0; i < start; ++i) {
                vertices.pop_front();
            }
        }
        int end = static_cast<int>(vertices.size()) - 1;
        assert(end >= 0);
        while (end >= 0 && (std::abs(vertices[end].x) > 1 || std::abs(vertices[end].y) > 1)) {
            --end;
        }
        if (end < 0) {
            // all points are outside the screen bounds
            clearVertexBufferImpl();
            return;
        }
        if (end < static_cast<int>(vertices.size()) - 1) {
            ++end;
            for (int i = static_cast<int>(vertices.size()) - 1; i > end; --i) {
                vertices.pop_back();
            }
        }

        // Create the vertex buffer
        std::vector<float> vertexBuffer;
        vertexBuffer.reserve(vertices.size() * 2);
        for (auto& p : vertices) {
            vertexBuffer.push_back(static_cast<float>(p.x));
            vertexBuffer.push_back(static_cast<float>(p.y));
        }
        updateVertexBufferImpl(vertexBuffer);
    }

    drawImpl();
}

void CustomBlueLine::clearCustomBlueLine() {
    std::lock_guard<std::mutex> lock(mux);
    line.clear();
    percent = 0;
    clearVertexBuffer = true;
}

void CustomBlueLine::setCustomBlueLine(LineString<double> line_) {
    std::lock_guard<std::mutex> lock(mux);
    line = std::move(line_);
    clearVertexBuffer = true;
}

void CustomBlueLine::setCustomBlueLinePercent(double percent_) {
    std::lock_guard<std::mutex> lock(mux);
    percent = std::clamp(percent_, 0.0, 1.0);
}

bool CustomBlueLine::transformChanged(const TransformState& transform) {
    if (line.empty()) {
        return false; // no points to compare
    }
    // reference point to compare to
    // use an actual point instead of an arbitrary constant
    LatLng ref = LatLng(line[0].y, line[0].x);
    // check if transform has changed such as a dot moved at least one pixel
    auto currentPoint = transform.latLngToScreenCoordinate(ref);
    auto currentPointX = static_cast<int>(std::round(currentPoint.x));
    auto currentPointY = static_cast<int>(std::round(currentPoint.y));
    auto changed = currentPointX != previousPointX || currentPointY != previousPointY || percent != previousPercent;
    previousPointX = currentPointX;
    previousPointY = currentPointY;
    previousPercent = percent;

    return changed;
}

} // namespace gfx
} // namespace mbgl
