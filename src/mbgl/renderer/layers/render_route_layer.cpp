#include "../../../../platform/glfw/ny_route.hpp"
#include "mbgl/renderer/buckets/route_bucket.hpp"

#include <mbgl/renderer/layers/render_route_layer.hpp>
#include <mbgl/style/layers/route_layer_properties.hpp>

#include <mbgl/geometry/feature_index.hpp>
#include <mbgl/programs/line_program.hpp>
#include <mbgl/programs/programs.hpp>
#include <mbgl/renderer/paint_parameters.hpp>
#include <mbgl/renderer/render_source.hpp>
#include <mbgl/renderer/upload_parameters.hpp>

#if MLN_DRAWABLE_RENDERER
#include <mbgl/gfx/drawable.hpp>
#include <mbgl/gfx/drawable_builder.hpp>
#include <mbgl/gfx/line_drawable_data.hpp>
#include <mbgl/renderer/layer_group.hpp>
#include <mbgl/renderer/layers/route_layer_tweaker.hpp>
#include <mbgl/renderer/update_parameters.hpp>
#include <mbgl/shaders/line_layer_ubo.hpp>
#include <mbgl/shaders/shader_program_base.hpp>

#endif

namespace mbgl {
using namespace style;
using namespace shaders;

namespace {

inline const RouteLayer::Impl& impl_cast(const Immutable<style::Layer::Impl>& impl) {
    assert(impl->getTypeInfo() == RouteLayer::Impl::staticTypeInfo());
    return static_cast<const RouteLayer::Impl&>(*impl);
}

#if MLN_DRAWABLE_RENDERER

const auto posNormalAttribName = "a_pos_normal";

#endif // MLN_DRAWABLE_RENDERER
} // namespace

RenderRouteLayer::RenderRouteLayer(Immutable<style::RouteLayer::Impl> _impl)
    : RenderLayer(makeMutable<RouteLayerProperties>(std::move(_impl))),
      unevaluated(impl_cast(baseImpl).paint.untransitioned()) {
    styleDependencies = unevaluated.getDependencies();
}

RenderRouteLayer::~RenderRouteLayer() = default;


void RenderRouteLayer::transition(const TransitionParameters& parameters) {
    unevaluated = impl_cast(baseImpl).paint.transitioned(parameters, std::move(unevaluated));
    // updateColorRamp();
}

void RenderRouteLayer::evaluate(const PropertyEvaluationParameters& parameters) {
    const auto previousProperties = staticImmutableCast<LineLayerProperties>(evaluatedProperties);
    auto properties = makeMutable<LineLayerProperties>(staticImmutableCast<LineLayer::Impl>(baseImpl),
                                                       parameters.getCrossfadeParameters(),
                                                       unevaluated.evaluate(parameters, previousProperties->evaluated));
    auto& evaluated = properties->evaluated;

    passes = (evaluated.get<style::LineOpacity>().constantOr(1.0) > 0 &&
              evaluated.get<style::LineColor>().constantOr(Color::black()).a > 0 &&
              evaluated.get<style::LineWidth>().constantOr(1.0) > 0)
                 ? RenderPass::Translucent
                 : RenderPass::None;
    properties->renderPasses = mbgl::underlying_type(passes);
    evaluatedProperties = std::move(properties);

#if MLN_DRAWABLE_RENDERER
    // if (auto* tweaker = static_cast<LineLayerTweaker*>(layerTweaker.get())) {
    //     tweaker->updateProperties(evaluatedProperties);
#if MLN_RENDER_BACKEND_METAL
        tweaker->updateGPUExpressions(unevaluated, parameters.now);
#endif // MLN_RENDER_BACKEND_METAL
    }
#endif

bool RenderRouteLayer::hasTransition() const {
    // return unevaluated.hasTransition();
    return false;
}

bool RenderRouteLayer::hasCrossfade() const {
    // return getCrossfade<LineLayerProperties>(evaluatedProperties).t != 1;
    return false;
}

namespace {

inline void setSegments(std::unique_ptr<gfx::DrawableBuilder>& builder, const RouteBucket& bucket) {
    builder->setSegments(gfx::Triangles(), bucket.sharedTriangles, bucket.segments.data(), bucket.segments.size());
}

} // namespace

void RenderRouteLayer::prepare(const LayerPrepareParameters& ) {
//DO nothing! Base class deals with tiles which we dont
}

void RenderRouteLayer::update(gfx::ShaderRegistry& shaders,
                             gfx::Context& context,
                             const TransformState&,
                             [[maybe_unused]] const std::shared_ptr<UpdateParameters>& parameters,
                             const RenderTree&,
                             UniqueChangeRequestVec& changes) {
    // Set up a layer group0.
    if (!layerGroup) {
        if (auto layerGroup_ = context.createLayerGroup(layerIndex, /*initialCapacity=*/64, getID())) {
            setLayerGroup(std::move(layerGroup_), changes);
        } else {
            return;
        }
    }
    // auto* layerGroupPtr = static_cast<LayerGroup*>(layerGroup.get());

    if (!layerTweaker) {
        auto tweaker = std::make_shared<RouteLayerTweaker>(getID(), evaluatedProperties);

        layerTweaker = std::move(tweaker);
        layerGroup->addLayerTweaker(layerTweaker);
    }

    if(!routeBucket) {
        routeBucket = std::make_shared<RouteBucket>();
    }

    if(routeBucket->getDirty()) {
        routeBucket->setDirty(false);
    }


    const RenderPass renderPass = static_cast<RenderPass>(evaluatedProperties->renderPasses &
                                                          ~mbgl::underlying_type(RenderPass::Opaque));

    auto createRouteBuilder = [&](const std::string& name,
                                 gfx::ShaderPtr shader) -> std::unique_ptr<gfx::DrawableBuilder> {
        std::unique_ptr<gfx::DrawableBuilder> builder = context.createDrawableBuilder(name);
        builder->setShader(std::static_pointer_cast<gfx::ShaderProgramBase>(shader));
        builder->setRenderPass(renderPass);
        builder->setSubLayerIndex(0);
        builder->setDepthType(gfx::DepthMaskType::ReadOnly);
        builder->setColorMode(renderPass == RenderPass::Translucent ? gfx::ColorMode::alphaBlended()
                                                                    : gfx::ColorMode::unblended());
        builder->setCullFaceMode(gfx::CullFaceMode::disabled());
        builder->setEnableStencil(true);

        return builder;
    };

    auto addRouteAttributes = [&](gfx::DrawableBuilder& builder, const RouteBucket& bucket, gfx::VertexAttributeArrayPtr&& vertexAttrs) {
        const auto vertexCount = bucket.vertices.elements();
        builder.setRawVertices({}, vertexCount, gfx::AttributeDataType::Short4);

        if (const auto& attr = vertexAttrs->set(idLinePosNormalVertexAttribute)) {
            attr->setSharedRawData(bucket.sharedVertices,
                                   offsetof(LineLayoutVertex, a1),
                                   /*vertexOffset=*/0,
                                   sizeof(LineLayoutVertex),
                                   gfx::AttributeDataType::Short2);
        }

        if (const auto& attr = vertexAttrs->set(idLineDataVertexAttribute)) {
            attr->setSharedRawData(bucket.sharedVertices,
                                   offsetof(LineLayoutVertex, a2),
                                   /*vertexOffset=*/0,
                                   sizeof(LineLayoutVertex),
                                   gfx::AttributeDataType::UByte4);
        }

        if(const auto& attr = vertexAttrs->set(idLineRouteDistanceToDestAttribute)) {
            // mbgl::gfx::VertexVector<float> dist2dest;
            const std::shared_ptr<mbgl::gfx::VertexVector<float>> dist2dest = std::make_shared<mbgl::gfx::VertexVector<float>>();
             size_t numverts = bucket.vertices.elements();
            dist2dest->reserve(numverts);
            float dist = float(numverts);
            for (size_t i = 0; i < numverts; ++i) {
                dist2dest->emplace_back((dist - float(i))/dist);
            }
            attr->setSharedRawData(dist2dest, 0, 0, sizeof(float), gfx::AttributeDataType::Float);
        }

        builder.setVertexAttributes(std::move(vertexAttrs));
    };

    StringIDSetsPair propertiesAsUniforms;

        auto& paintPropertyBinders = routeBucket->paintPropertyBinders.at(getID());
        const auto& evaluated = getEvaluated<RouteLayerProperties>(evaluatedProperties);

        // auto updateExisting = [&](gfx::Drawable& drawable) {
        //     if (drawable.getLayerTweaker() != layerTweaker) {
        //         // This drawable was produced on a previous style/bucket, and should not be updated.
        //         return false;
        //     }
        //     return true;
        // };

        const auto addDrawable = [&](std::unique_ptr<gfx::Drawable>& drawable) {
            drawable->setLayerTweaker(layerTweaker);
            drawable->setBinders(routeBucket, &paintPropertyBinders);

            // const bool roundCap = LineCapType::Round;
            // const auto capType = roundCap ? LinePatternCap::Round : LinePatternCap::Square;
            // drawable->setData(std::make_unique<gfx::LineDrawableData>(LineCapType::Round));

            layerGroup->addDrawable(drawable);
            ++stats.drawablesAdded;
        };

        propertiesAsUniforms.first.clear();
        propertiesAsUniforms.second.clear();

        auto vertexAttrs = context.createVertexAttributeArray();
        vertexAttrs->readDataDrivenPaintProperties<LineColor,
                                                   LineBlur,
                                                   LineOpacity,
                                                   LineGapWidth,
                                                   LineOffset,
                                                   LineWidth,
                                                   LineFloorWidth,
                                                   LinePattern>(
            paintPropertyBinders, evaluated, propertiesAsUniforms, idLineColorVertexAttribute);

            if(!routeShaderGroup) {
                routeShaderGroup = shaders.getShaderGroup("LineRouteShader");
                if(!routeShaderGroup) {
                    //TODO: need to throw an exception
                }
            }

            auto shader = routeShaderGroup->getOrCreateShader(context, propertiesAsUniforms, posNormalAttribName);
            if(!shader) {
                //TODO: need to throw an exception
            }

            //build the drawables.
            auto builder = createRouteBuilder("routeLine", std::move(shader));

            addRouteAttributes(*builder, *routeBucket, std::move(vertexAttrs));
            setSegments(builder, *routeBucket);
            builder->flush(context);
            for (auto& drawable : builder->clearDrawables()) {
                addDrawable(drawable);
            }

}

}


