
#include <mbgl/gfx/custom_puck_style.hpp>
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

std::string parseIconName(const auto& icon, const auto& iconNames) {
    if (!icon.IsString()) {
        throw std::runtime_error("icon name must be a string");
    }
    auto iconName = icon.GetString();
    if (iconNames.find(iconName) == iconNames.end()) {
        throw std::runtime_error(std::string("Icon name not found: ") + iconName);
    }
    return iconName;
}

bool parseBool(const auto& value) {
    if (!value.IsBool()) {
        throw std::runtime_error("Value must be a boolean");
    }
    return value.GetBool();
}

float parseNumber(const auto& value) {
    if (!value.IsNumber()) {
        throw std::runtime_error("Value must be a number");
    }
    return value.GetFloat();
}

CustomPuckSampler<float> parseSampler(const auto& value) {
    CustomPuckSampler<float> sampler;
    CustomPuckValue<float> v;
    if (!value.IsArray()) {
        v.time = 0;
        v.value = parseNumber(value);
        sampler.push_back(v);
        return sampler;
    }
    if (value.Size() % 2 != 0) {
        throw std::runtime_error("Expecting array of pairs");
    }
    for (rapidjson::SizeType i = 0; i < value.Size(); i += 2) {
        if (!value[i].IsNumber() || !value[i + 1].IsNumber()) {
            throw std::runtime_error("Expecting array of pairs");
        }
        v.time = parseNumber(value[i]);
        v.value = parseNumber(value[i + 1]);
        sampler.push_back(v);
    }
    return sampler;
}

Color parseColor(const auto& value) {
    if (!value.IsArray() && value.Size() != 3) {
        throw std::runtime_error("Color must be an array of 3 numbers");
    }
    Color color;
    color.r = parseNumber(value[0]);
    color.g = parseNumber(value[1]);
    color.b = parseNumber(value[2]);
    color.a = 1.0f;
    return color;
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

CustomPuckIconStyle parseIconStyle(const auto& icon, const auto& iconNames, const std::string& prefix) {
    CustomPuckIconStyle iconStyle;
    if (!icon.IsObject()) {
        throw std::runtime_error("The state icon variant must be an object");
    }
    if (icon.HasMember(prefix.c_str())) {
        iconStyle.name = parseIconName(icon[prefix.c_str()], iconNames);
    }
    if (icon.HasMember((prefix + "_color").c_str())) {
        iconStyle.color = parseColor(icon[(prefix + "_color").c_str()]);
    }
    if (icon.HasMember((prefix + "_scale").c_str())) {
        iconStyle.scale = parseSampler(icon[(prefix + "_scale").c_str()]);
    }
    if (icon.HasMember((prefix + "_opacity").c_str())) {
        iconStyle.opacity = parseSampler(icon[(prefix + "_opacity").c_str()]);
    }
    if (icon.HasMember((prefix + "_mirror_x").c_str())) {
        iconStyle.mirrorX = parseBool(icon[(prefix + "_mirror_x").c_str()]);
    }
    if (icon.HasMember((prefix + "_mirror_y").c_str())) {
        iconStyle.mirrorY = parseBool(icon[(prefix + "_mirror_y").c_str()]);
    }
    return iconStyle;
}

CustomPuckIconsStyle parseIconsStyle(const auto& icon, const auto& iconNames) {
    CustomPuckIconsStyle iconsStyle;
    iconsStyle.icon1 = parseIconStyle(icon, iconNames, "icon_1");
    iconsStyle.icon2 = parseIconStyle(icon, iconNames, "icon_2");
    if (icon.HasMember("animation_duration")) {
        iconsStyle.icon1->animationDuration = parseNumber(icon["animation_duration"]);
        iconsStyle.icon2->animationDuration = iconsStyle.icon1->animationDuration;
        if (icon.HasMember("animation_loop")) {
            iconsStyle.icon1->animationLoop = parseBool(icon["animation_loop"]);
            iconsStyle.icon2->animationLoop = iconsStyle.icon1->animationLoop;
        }
    }
    return iconsStyle;
}

void mergeIconStyle(const CustomPuckIconStyle& defaultStyle, CustomPuckIconStyle& style) {
    if (style.scale.empty()) {
        style.scale = defaultStyle.scale;
    }
    if (style.opacity.empty()) {
        style.opacity = defaultStyle.opacity;
    }
    if (!style.color) {
        style.color = defaultStyle.color;
    }
    if (!style.animationDuration) {
        style.animationDuration = defaultStyle.animationDuration;
    }
    if (!style.animationLoop) {
        style.animationLoop = defaultStyle.animationLoop;
    }
    if (!style.mirrorX) {
        style.mirrorX = defaultStyle.mirrorX;
    }
    if (!style.mirrorY) {
        style.mirrorY = defaultStyle.mirrorY;
    }
}

void mergeIconsStyle(const CustomPuckIconsStyle& defaultStyle, CustomPuckIconsStyle& style) {
    if (!style.icon1) {
        style.icon1 = defaultStyle.icon1;
    } else if (defaultStyle.icon1) {
        mergeIconStyle(*defaultStyle.icon1, *style.icon1);
    }
    if (style.icon1 && style.icon1->name.empty()) {
        style.icon1 = std::nullopt;
    }
    if (!style.icon2) {
        style.icon2 = defaultStyle.icon2;
    } else if (defaultStyle.icon2) {
        mergeIconStyle(*defaultStyle.icon2, *style.icon2);
    }
    if (style.icon2 && style.icon2->name.empty()) {
        style.icon2 = std::nullopt;
    }
}

std::unordered_map<CustomPuckStyleVariantName, CustomPuckIconsStyle> parseState(const auto& state,
                                                                                const auto& variantNames,
                                                                                const auto& iconNames) {
    std::unordered_map<CustomPuckStyleVariantName, CustomPuckIconsStyle> result;
    // Default variant
    CustomPuckIconsStyle defaultIconsStyle;
    for (const auto& variant : state.GetArray()) {
        if (!variant.IsObject()) {
            throw std::runtime_error("Expecting variant objects");
        }
        if (!variant.HasMember("variant")) {
            defaultIconsStyle = parseIconsStyle(variant, iconNames);
            break;
        }
    }
    // Set all variants to default
    for (const auto& variantName : variantNames) {
        result[variantName] = defaultIconsStyle;
    }
    // Other variants
    for (const auto& variant : state.GetArray()) {
        if (!variant.IsObject()) {
            throw std::runtime_error("Expecting variant objects");
        }
        if (!variant.HasMember("variant")) {
            // default variant already parsed
            continue;
        }
        if (!variant["variant"].IsString()) {
            throw std::runtime_error("variant names must be strings");
        }
        auto variantName = variant["variant"].GetString();
        auto iconsStyle = parseIconsStyle(variant, iconNames);
        mergeIconsStyle(defaultIconsStyle, iconsStyle);
        result[variantName] = std::move(iconsStyle);
    }
    return result;
}

CustomPuckStyleVariantMap parseVariants(const auto& states, const auto& variantNames, const auto& iconNames) {
    CustomPuckStyleVariantMap variants;
    for (auto it = states.MemberBegin(); it != states.MemberEnd(); ++it) {
        if (!it->name.IsString() || !it->value.IsArray()) {
            throw std::runtime_error("A state name a string and a state is an array of variants");
        }
        auto stateName = it->name.GetString();
        auto stateVariants = parseState(it->value, variantNames, iconNames);
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

rapidjson::Document parseDoc(const std::string& json) {
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

CustomPuckStyle parseCustomPuckStyle(const std::string& json) {
    auto doc = parseDoc(json);
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
