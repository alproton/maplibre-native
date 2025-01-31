#include <mbgl/layermanager/route_layer_factory.hpp>
#include <mbgl/style/layers/route_layer.hpp>
#include <mbgl/style/layers/route_layer_impl.hpp>
#include <mbgl/renderer/layers/render_route_layer.hpp>

namespace mbgl {

const style::LayerTypeInfo* RouteLayerFactory::getTypeInfo() const noexcept {
    return style::RouteLayer::Impl::staticTypeInfo();
}

std::unique_ptr<style::Layer> RouteLayerFactory::createLayer(const std::string& id,
                                          const style::conversion::Convertible& value) noexcept {

    const auto source = getSource(value);
    if (!source) {
        return nullptr;
    }
    return std::unique_ptr<style::Layer>(new (std::nothrow) style::RouteLayer(id, *source));

}

std::unique_ptr<RenderLayer> RouteLayerFactory::createRenderLayer(Immutable<style::Layer::Impl> impl) noexcept {
    assert(impl->getTypeInfo() == getTypeInfo());
    return std::make_unique<RenderRouteLayer>(staticImmutableCast<style::RouteLayer::Impl>(std::move(impl)));
    // return std::unique_ptr<RenderLayer>(new (std::nothrow) RenderRouteLayer(std::move(impl)));
}


}