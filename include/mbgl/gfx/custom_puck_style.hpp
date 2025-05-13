#pragma once

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>
#include <mbgl/math/clamp.hpp>
#include <mbgl/util/color.hpp>

namespace mbgl {
namespace gfx {

using CustomPuckIconName = std::string;
using CustomPuckIconPath = std::string;
using CustomPuckStateName = std::string;
using CustomPuckStyleVariantName = std::string;

enum class CustomPuckIconAnimationType {
    None,
    Loop,
};

template <typename T>
struct CustomPuckValue {
    float time;
    T value;
};

template <typename T>
class CustomPuckSampler : public std::vector<CustomPuckValue<T>> {
public:
    using Base = std::vector<CustomPuckValue<T>>;
    using Base::Base;

    T sample(float time) const {
        if (this->empty()) {
            return T(0);
        }

        auto it = std::lower_bound(this->begin(), this->end(), time, [](const CustomPuckValue<T>& sample, float time) {
            return sample.time < time;
        });

        if (it == this->end()) {
            return this->back().value;
        }

        if (it == this->begin()) {
            return this->front().value;
        }

        const auto& a = *(it - 1);
        const auto& b = *it;
        float t = (time - a.time) / (b.time - a.time);
        return a.value * (1 - t) + b.value * t;
    }
};

struct CustomPuckIconStyle {
    CustomPuckSampler<float> scale;
    CustomPuckSampler<float> opacity;

    CustomPuckIconName name;
    Color color;
    float animationDuration = 0;
    CustomPuckIconAnimationType animationType = CustomPuckIconAnimationType::None;
    bool mirrorX = false;
    bool mirrorY = false;

    float sampleScale(float time) const { return scale.sample(remapTime(time)); }

    float sampleOpacity(float time) const { return opacity.sample(remapTime(time)); }

private:
    float remapTime(float time) const {
        switch (animationType) {
            case CustomPuckIconAnimationType::None:
                time = util::clamp(time, 0.f, 1.f);
                break;
            case CustomPuckIconAnimationType::Loop:
                time = std::fmod(time, 1.f);
                break;
            default:
                break;
        }
        return time * animationDuration;
    }
};

struct CustomPuckIconsStyle {
    CustomPuckIconStyle icon1;
    CustomPuckIconStyle icon2;
};

using CustomPuckStyleVariant = std::unordered_map<CustomPuckStateName, CustomPuckIconsStyle>;
using CustomPuckStyleVariants = std::unordered_map<CustomPuckStyleVariantName, CustomPuckStyleVariant>;

struct CustomPuckStyle {
    std::unordered_map<CustomPuckStyleVariantName, CustomPuckStyleVariant> variants;
    std::unordered_map<CustomPuckIconName, CustomPuckIconPath> icons;
};

CustomPuckStyle parseCustomPuckStyle(const std::string& style_file_path);

} // namespace gfx
} // namespace mbgl
