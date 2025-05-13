
#include <mbgl/gfx/custom_puck_style.hpp>
#include <mbgl/util/io.hpp>
#include <rapidjson/document.h>
#include <vector>

namespace mbgl {
namespace gfx {

namespace {

void checkMember(const auto& value, const char* name) {
    if (!value.HasMember(name)) {
        throw std::runtime_error(std::string("Missing member: ") + name);
    }
}

void checkArrayMember(const auto& value, const char* name) {
    checkMember(value, name);
    if (!value[name].IsArray()) {
        throw std::runtime_error(std::string("Member is not an array: ") + name);
    }
}

void checkObjectMember(const auto& value, const char* name) {
    checkMember(value, name);
    if (!value[name].IsObject()) {
        throw std::runtime_error(std::string("Member is not an object: ") + name);
    }
}

std::vector<std::string> parseVariantNames(const auto& variants) {
    std::vector<std::string> names;
    for (const auto& variant : variants.GetArray()) {
        if (!variant.IsString()) {
            throw std::runtime_error("Variant name is not a string");
        }
        names.push_back(variant.GetString());
    }
    return names;
}

} // namespace

CustomPuckStyle parseCustomPuckStyle(const std::string& style_file_path) {
    rapidjson::Document doc;
    doc.Parse(util::read_file(style_file_path).c_str());
    checkArrayMember(doc, "variants");
    checkArrayMember(doc, "icons");
    checkObjectMember(doc, "states");

    auto variantNames = parseVariantNames(doc["variants"]);
    if (variantNames.empty()) {
        throw std::runtime_error("No variants found");
    }

    return {};
}

} // namespace gfx
} // namespace mbgl
