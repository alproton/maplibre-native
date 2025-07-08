
#pragma once
#include <mbgl/shaders/shader_source.hpp>

namespace mbgl {
namespace shaders {

template <>
struct ShaderSource<BuiltIn::CustomDotsProgram, gfx::Backend::Type::OpenGL> {
    static constexpr const char* name = "CustomDotsProgram";
    static constexpr const char* vertex = R"(#version 310 es
layout (location = 0) in vec2 pos;
layout(location = 1) uniform vec2 size;
out mediump vec2 uv;
void main() {
  mediump float dx = size.x;
  mediump float dy = size.y;
  vec2 trianglePos[6] = vec2[6](vec2(-dx,-dy),
                                vec2(+dx,-dy),
                                vec2(-dx,+dy),
                                vec2(-dx,+dy),
                                vec2(+dx,-dy),
                                vec2(+dx,+dy));
  vec2 triangleUv[6] = vec2[6](vec2(0, 0),
                               vec2(1, 0),
                               vec2(0, 1),
                               vec2(0, 1),
                               vec2(1, 0),
                               vec2(1, 1));
  gl_Position = vec4(pos + trianglePos[gl_VertexID], 0, 1);
  uv = triangleUv[gl_VertexID];
}
)";
    static constexpr const char* fragment = R"(#version 310 es
out mediump vec4 fragColor;
in mediump vec2 uv;
layout(location = 2) uniform mediump vec3 innerColor;
layout(location = 3) uniform mediump vec3 outerColor;
layout(location = 4) uniform mediump float innerFactor;
void main() {
  mediump vec2 p = uv * 2.0 - vec2(1.0);
  mediump float outer = 1.0 - min(pow(dot(p, p), 4.0), 1.0);
  mediump float inner = 1.0 - min(pow(dot(p, p) * innerFactor, 4.0), 1.0);
  mediump vec3 color = mix(outerColor, innerColor, inner);
  fragColor = vec4(color, 1.0) * outer;
}
)";
};

} // namespace shaders
} // namespace mbgl
