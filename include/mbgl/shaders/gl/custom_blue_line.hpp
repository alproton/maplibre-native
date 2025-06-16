
#pragma once
#include <mbgl/shaders/shader_source.hpp>

namespace mbgl {
namespace shaders {

template <>
struct ShaderSource<BuiltIn::CustomBlueLineProgram, gfx::Backend::Type::OpenGL> {
    static constexpr const char* name = "CustomBlueLineProgram";
    static constexpr const char* vertex = R"(
#version 310 es
layout (location = 0) in vec2 pos;
void main() {
  gl_Position = vec4(pos, 0, 1);
}
)";
    static constexpr const char* fragment = R"(
#version 310 es
out mediump vec4 fragColor;
layout(location = 0) uniform mediump vec4 color;
void main() {
  fragColor = color;
}
)";
};

} // namespace shaders
} // namespace mbgl
