
#include <mbgl/gfx/custom_puck_style.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/logging.hpp>
#include <rapidjson/document.h>
#include "rapidjson/error/en.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

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

std::vector<CustomPuckStyleVariantName> parseVariantNames(const auto& variants) {
    std::vector<CustomPuckStyleVariantName> names;
    for (const auto& variant : variants.GetArray()) {
        if (!variant.IsString()) {
            throw std::runtime_error("Variant name is not a string");
        }
        names.push_back(variant.GetString());
    }
    if (names.empty()) {
        throw std::runtime_error("No variants found");
    }
    return names;
}

std::unordered_map<CustomPuckIconName, CustomPuckIconPath> parseIconsNames(const auto& icons) {
    std::unordered_map<CustomPuckIconName, CustomPuckIconPath> names;
    for (const auto& icon : icons.GetArray()) {
        if (!icon.IsObject()) {
            throw std::runtime_error("Expecting icon name/path pair objects");
        }
        for (auto it = icon.MemberBegin(); it != icon.MemberEnd(); ++it) {
            if (!it->name.IsString() || !it->value.IsString()) {
                throw std::runtime_error("A pair of icon name and icon path pair is expected in the icons array");
            }
            names[it->name.GetString()] = it->value.GetString();
        }
    }
    if (names.empty()) {
        throw std::runtime_error("No icons found");
    }
    return names;
}

CustomPuckIconsStyle parseIconsStyle(const auto& icon, const auto& iconNames) {
    if (!icon.IsObject()) {
        throw std::runtime_error("The state icon variant must be an object");
    }
    if (icon.HasMember("icon_1")) {
        auto iconName = icon["icon_1"].GetString();
        if (iconNames.find(iconName) == iconNames.end()) {
            throw std::runtime_error(std::string("Icon name not found: ") + iconName);
        }
    }
    return {};
}

std::unordered_map<CustomPuckStyleVariantName, CustomPuckIconsStyle> parseState(const auto& state,
                                                                                const auto& iconNames) {
    // Default variant
    CustomPuckIconsStyle defaultIconsStyle;
    for (const auto& variant : state.GetArray()) {
        if (!variant.IsObject()) {
            throw std::runtime_error("Expecting variant objects");
        }
        if (variant.HasMember("variant")) {
            defaultIconsStyle = parseIconsStyle(variant, iconNames);
        }
    }
    return {};
}

CustomPuckStyleVariants parseVariants(const auto& states, const auto& variantNames, const auto& iconNames) {
    CustomPuckStyleVariants variants;
    for (auto it = states.MemberBegin(); it != states.MemberEnd(); ++it) {
        if (!it->name.IsString() || !it->value.IsArray()) {
            throw std::runtime_error("A state name a string and a state is an array of variants");
        }
        auto stateName = it->name.GetString();
        auto stateVariants = parseState(it->value, iconNames);
        for (auto& state : stateVariants) {
            if (variantNames.find(state.first) == variantNames.end()) {
                throw std::runtime_error(
                    "State using an invalid variant. Make sure all variants are defined in the variants array");
            }
            variants[state.first][stateName] = std::move(state.second);
        }
    }
    return variants;
}

std::pair<int, int> offsetToLineColumn(const std::string& json, size_t offset) {
    int line = 1;
    int column = 1;
    for (size_t i = 0; i < offset && i < json.length(); ++i) {
        if (json[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }
    return {line, column};
}

rapidjson::Document parseDoc(const std::string& style_file_path) {
    auto json = util::read_file(style_file_path);
    rapidjson::Document doc;
    rapidjson::ParseResult parseOk = doc.Parse(json.c_str());
    if (!parseOk) {
        auto [line, column] = offsetToLineColumn(json, parseOk.Offset());
        auto msg = std::string("Error parsing puck style\n line: ") + std::to_string(line) +
                   " column: " + std::to_string(column) + std::string(" error: ") + GetParseError_En(parseOk.Code());
        Log::Error(Event::General, msg);
        throw std::runtime_error(msg);
    }
    return doc;
}

} // namespace

CustomPuckStyle parseCustomPuckStyle(const std::string& style_file_path) {
    auto doc = parseDoc(style_file_path);
    checkArrayMember(doc, "variants");
    checkArrayMember(doc, "icons");
    checkObjectMember(doc, "states");
    auto variantNames = parseVariantNames(doc["variants"]);
    std::unordered_set<std::string> variantsSet(variantNames.begin(), variantNames.end());
    auto icons = parseIconsNames(doc["icons"]);
    auto variants = parseVariants(doc["states"], variantsSet, icons);
    CustomPuckStyle style;
    style.variants = std::move(variants);
    style.icons = std::move(icons);
    return style;
}

} // namespace gfx
} // namespace mbgl
