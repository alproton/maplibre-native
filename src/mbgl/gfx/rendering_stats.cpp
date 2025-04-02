#include <mbgl/gfx/rendering_stats.hpp>

#include <algorithm>
#include <initializer_list>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace mbgl {
namespace gfx {

bool RenderingStats::isZero() const {
    const auto expectedZeros = {numActiveTextures,
                                numTextureBindings,
                                numBuffers,
                                numVertexBuffers,
                                numIndexBuffers,
                                numUniformBuffers,
                                numFrameBuffers,
                                memTextures,
                                memBuffers,
                                memIndexBuffers,
                                memVertexBuffers,
                                memUniformBuffers};
    return std::all_of(expectedZeros.begin(), expectedZeros.end(), [](auto x) { return x == 0; });
}

RenderingStats& RenderingStats::operator+=(const RenderingStats& r) {
    numFrames += r.numFrames;
    numDrawCalls += r.numDrawCalls;
    totalDrawCalls += r.totalDrawCalls;
    numCreatedTextures += r.numCreatedTextures;
    numActiveTextures += r.numActiveTextures;
    numTextureBindings += r.numTextureBindings;
    numTextureUpdates += r.numTextureUpdates;
    textureUpdateBytes += r.textureUpdateBytes;
    totalBuffers += r.totalBuffers;
    totalBufferObjs += r.totalBufferObjs;
    bufferUpdates += r.bufferUpdates;
    bufferObjUpdates += r.bufferObjUpdates;
    bufferUpdateBytes += r.bufferUpdateBytes;
    numBuffers += r.numBuffers;
    numFrameBuffers += r.numFrameBuffers;
    numIndexBuffers += r.numIndexBuffers;
    indexUpdateBytes += r.indexUpdateBytes;
    numVertexBuffers += r.numVertexBuffers;
    vertexUpdateBytes += r.vertexUpdateBytes;
    numUniformBuffers += r.numUniformBuffers;
    numUniformUpdates += r.numUniformUpdates;
    uniformUpdateBytes += r.uniformUpdateBytes;
    memTextures += r.memTextures;
    memBuffers += r.memBuffers;
    memIndexBuffers += r.memIndexBuffers;
    memVertexBuffers += r.memVertexBuffers;
    memUniformBuffers += r.memUniformBuffers;
    stencilClears += r.stencilClears;
    stencilUpdates += r.stencilUpdates;
    return *this;
}

std::string RenderingStats::toJSONString() const {
    rapidjson::Document document;
    document.SetObject();

    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    rapidjson::Value rendering_stats(rapidjson::kObjectType);
    rendering_stats.AddMember("numDrawCalls", numDrawCalls, allocator);
    rendering_stats.AddMember("numCreatedTextures", numCreatedTextures, allocator);
    rendering_stats.AddMember("numActiveTextures", numActiveTextures, allocator);
    rendering_stats.AddMember("numBuffers", numBuffers, allocator);
    rendering_stats.AddMember("memTextures", memTextures, allocator);
    rendering_stats.AddMember("memIndexBuffers", memIndexBuffers, allocator);
    rendering_stats.AddMember("memVertexBuffers", memVertexBuffers, allocator);
    rendering_stats.AddMember("stencilUpdates", stencilUpdates, allocator);

    document.AddMember("rendering_stats", rendering_stats, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    return buffer.GetString();
}

#if !defined(NDEBUG)
template <typename T>
std::ostream& optionalStatLine(std::ostream& stream, T value, std::string_view label, std::string_view sep) {
    if (value) {
        stream << label << " = " << value << sep;
    }
    return stream;
}
std::string RenderingStats::toString(std::string_view sep) const {
    std::stringstream ss;
    optionalStatLine(ss, numFrames, "numFrames", sep);
    optionalStatLine(ss, numDrawCalls, "numDrawCalls", sep);
    optionalStatLine(ss, totalDrawCalls, "totalDrawCalls", sep);
    optionalStatLine(ss, numCreatedTextures, "numCreatedTextures", sep);
    optionalStatLine(ss, numActiveTextures, "numActiveTextures", sep);
    optionalStatLine(ss, numTextureBindings, "numTextureBindings", sep);
    optionalStatLine(ss, numTextureUpdates, "numTextureUpdates", sep);
    optionalStatLine(ss, textureUpdateBytes, "textureUpdateBytes", sep);
    optionalStatLine(ss, totalBuffers, "totalBuffers", sep);
    optionalStatLine(ss, totalBufferObjs, "totalBufferObjs", sep);
    optionalStatLine(ss, bufferUpdates, "bufferUpdates", sep);
    optionalStatLine(ss, bufferObjUpdates, "bufferObjUpdates", sep);
    optionalStatLine(ss, bufferUpdateBytes, "bufferUpdateBytes", sep);
    optionalStatLine(ss, numBuffers, "numBuffers", sep);
    optionalStatLine(ss, numFrameBuffers, "numFrameBuffers", sep);
    optionalStatLine(ss, numIndexBuffers, "numIndexBuffers", sep);
    optionalStatLine(ss, indexUpdateBytes, "indexUpdateBytes", sep);
    optionalStatLine(ss, numVertexBuffers, "numVertexBuffers", sep);
    optionalStatLine(ss, vertexUpdateBytes, "vertexUpdateBytes", sep);
    optionalStatLine(ss, numUniformBuffers, "numUniformBuffers", sep);
    optionalStatLine(ss, numUniformUpdates, "numUniformUpdates", sep);
    optionalStatLine(ss, uniformUpdateBytes, "uniformUpdateBytes", sep);
    optionalStatLine(ss, memTextures, "memTextures", sep);
    optionalStatLine(ss, memBuffers, "memBuffers", sep);
    optionalStatLine(ss, memIndexBuffers, "memIndexBuffers", sep);
    optionalStatLine(ss, memVertexBuffers, "memVertexBuffers", sep);
    optionalStatLine(ss, memUniformBuffers, "memUniformBuffers", sep);
    optionalStatLine(ss, stencilClears, "stencilClears", sep);
    optionalStatLine(ss, stencilUpdates, "stencilUpdates", sep);
    return ss.str();
}
#endif

} // namespace gfx
} // namespace mbgl
