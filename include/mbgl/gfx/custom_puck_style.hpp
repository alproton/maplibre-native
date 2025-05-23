#pragma once

#include <cmath>
#include <optional>
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
    std::optional<Color> color;
    std::optional<float> animationDuration;
    std::optional<bool> animationLoop;
    std::optional<bool> mirrorX;
    std::optional<bool> mirrorY;

    float sampleScale(float time) const { return scale.empty() ? 1 : scale.sample(remapTime(time)); }

    float sampleOpacity(float time) const { return opacity.empty() ? 1 : opacity.sample(remapTime(time)); }

private:
    float remapTime(float time) const {
        if (!animationDuration) {
            return 0;
        }
        time /= *animationDuration;
        if (animationLoop.value_or(false)) {
            time = std::fmod(time, 1.f);
        } else {
            time = util::clamp(time, 0.f, 1.f);
        }
        return time;
    }
};

struct CustomPuckIconsStyle {
    std::optional<CustomPuckIconStyle> icon1;
    std::optional<CustomPuckIconStyle> icon2;
};

using CustomPuckStyleVariant = std::unordered_map<CustomPuckStateName, CustomPuckIconsStyle>;
using CustomPuckStyleVariantMap = std::unordered_map<CustomPuckStyleVariantName, CustomPuckStyleVariant>;
using CustomPuckIconMap = std::unordered_map<CustomPuckIconName, CustomPuckIconPath>;

struct CustomPuckStyle {
    CustomPuckStyleVariantMap variants;
    CustomPuckIconMap icons;
};

CustomPuckStyle parseCustomPuckStyle(const std::string& json);

} // namespace gfx
} // namespace mbgl
