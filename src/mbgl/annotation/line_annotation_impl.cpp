#include "mbgl/style/layers/line_layer_impl.hpp"

#include <mbgl/annotation/line_annotation_impl.hpp>
#include <mbgl/annotation/annotation_manager.hpp>
#include <mbgl/style/style_impl.hpp>
#include <mbgl/style/layers/line_layer.hpp>
#include <mbgl/style/layers/route_layer.hpp>

namespace mbgl {

using namespace style;

LineAnnotationImpl::LineAnnotationImpl(AnnotationID id_, LineAnnotation annotation_)
    : ShapeAnnotationImpl(id_),
      annotation(ShapeAnnotationGeometry::visit(annotation_.geometry, CloseShapeAnnotation{}),
                 annotation_.opacity,
                 annotation_.width,
                 annotation_.color,
                 annotation_.isRoute) {}

void LineAnnotationImpl::updateStyle(Style::Impl& style) const {
    Layer* layer = style.getLayer(layerID);

    bool enableNewRouteImpl = false;

    if (!layer) {
        if(enableNewRouteImpl && annotation.isRoute) {
            auto newLayer = std::make_unique<RouteLayer>(layerID, AnnotationManager::SourceID);
            newLayer->setSourceLayer(layerID);
            newLayer->setLineJoin(LineJoinType::Round);
            layer = style.addLayer(std::move(newLayer), AnnotationManager::PointLayerID);
        }
        else {
            auto newLayer = std::make_unique<LineLayer>(layerID, AnnotationManager::SourceID);
            newLayer->setIsRoute(annotation.isRoute);
            newLayer->setSourceLayer(layerID);
            newLayer->setLineJoin(LineJoinType::Round);
            layer = style.addLayer(std::move(newLayer), AnnotationManager::PointLayerID);
        }
    }

    if(enableNewRouteImpl && annotation.isRoute) {
        auto* routeLayer = static_cast<RouteLayer*>(layer);
        routeLayer->setLineOpacity(annotation.opacity);
        routeLayer->setLineWidth(annotation.width);
        routeLayer->setLineColor(annotation.color);
    }
    else {
        auto* lineLayer = static_cast<LineLayer*>(layer);
        lineLayer->setLineOpacity(annotation.opacity);
        lineLayer->setLineWidth(annotation.width);
        lineLayer->setLineColor(annotation.color);
    }
}

const ShapeAnnotationGeometry& LineAnnotationImpl::geometry() const {
    return annotation.geometry;
}

} // namespace mbgl
