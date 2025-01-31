#include <mbgl/style/layers/route_layer_impl.hpp>

namespace mbgl {
namespace style {

bool RouteLayer::Impl::hasLayoutDifference(const Layer::Impl& other) const {
    assert(other.getTypeInfo() == getTypeInfo());
    const auto& impl = static_cast<const style::RouteLayer::Impl&>(other);
    return filter != impl.filter || visibility != impl.visibility || layout != impl.layout ||
           paint.hasDataDrivenPropertyDifference(impl.paint);
}

} // namespace style
} // namespace mbgl
