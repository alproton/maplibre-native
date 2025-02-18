#include <mbgl/gfx/custom_puck.hpp>

namespace mbgl {
namespace gfx {

namespace {

CustomPuckState& customPuckGlobalState() {
    static CustomPuckState state;
    return state;
}

} // namespace

void customPuckSetState(const CustomPuckState& state) {
    customPuckGlobalState() = state;
}

const CustomPuckState& customPuckGetState() {
    return customPuckGlobalState();
}

} // namespace gfx
} // namespace mbgl
