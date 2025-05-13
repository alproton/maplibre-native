#pragma once

#include <mbgl/gfx/custom_puck.hpp>
#include <mbgl/gl/context.hpp>
#include <unordered_map>

namespace mbgl {
namespace gl {

class CustomPuck final : public gfx::CustomPuck {
public:
    CustomPuck(gl::Context&);

    ~CustomPuck() noexcept override;

    void drawImpl(const gfx::CustomPuckSampledStyle& style) override;
    gfx::CustomPuckState getState() override;

private:
    void updateTextures(const gfx::CustomPuckIconMap& icons);
    void draw(const gfx::CustomPuckSampledIcon& icon) const;

private:
    gl::Context& context;
    UniqueProgram program;
    std::unordered_map<gfx::CustomPuckIconName, TextureID> textures;
    int storage = 0;
};

} // namespace gl
} // namespace mbgl
