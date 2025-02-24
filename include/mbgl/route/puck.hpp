#pragma once

#include <string>
#include <mbgl/style/image.hpp>
#include <mbgl/style/expression/image.hpp>

namespace mbgl {
namespace route {

enum PuckImageType {
    pitTop,
    pitShadow,
    pitBearing,
    pitInvalid
};

struct PuckImage {
    std::string fileName;
    std::string fileLocation;
};

struct PuckOptions {
    std::unordered_map<PuckImageType, PuckImage> locations;
};

class Puck {
    public:
        Puck() = default;
        Puck(const PuckOptions& popts);
        PuckOptions getPuckOptions() const;
        void setVisible(bool onOff);
        bool isVisible() const;

    private:
        PuckOptions popts_;
        bool visibile_ = false;
};

}
}

