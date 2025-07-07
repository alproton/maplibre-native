
#pragma once
#include <mbgl/shaders/shader_source.hpp>

namespace mbgl {
namespace shaders {

template <>
struct ShaderSource<BuiltIn::CustomPuckProgram, gfx::Backend::Type::OpenGL> {
    static constexpr const char* name = "CustomPuckProgram";
    static constexpr const char* vertex = R"(#version 310 es
// Since this is a temporary workaround to issue #3135, we use
// uniform vectors for the puck quad to simplify the GL side code
layout(location = 0) uniform vec2 v0;
layout(location = 1) uniform vec2 v1;
layout(location = 2) uniform vec2 v2;
layout(location = 3) uniform vec2 v3;
out mediump vec2 uv;
void main() {
  vec2 pos[4] = vec2[4](v0, v1, v2, v3);
  vec2 vertex_uv[4] = vec2[4](vec2(0,1), vec2(1,1), vec2(0,0), vec2(1,0));
  gl_Position = vec4(pos[gl_VertexID], 0, 1);
  uv = vertex_uv[gl_VertexID];
}
)";
    static constexpr const char* fragment = R"(#version 310 es
in mediump vec2 uv;
out mediump vec4 fragColor;
uniform sampler2D tex;
layout(location = 4) uniform mediump vec4 color;
void main() {
  fragColor = texture(tex, uv) * color;
}
)";
};

} // namespace shaders
} // namespace mbgl
