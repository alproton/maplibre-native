#include <mbgl/route/puck.hpp>

namespace mbgl {
namespace route {

Puck::Puck(const PuckOptions& popts) : popts_(popts) {
}

PuckOptions Puck::getPuckOptions() const {
    return popts_;
}

void Puck::setVisible(bool onOff) {
    visibile_ = onOff;
}

bool Puck::isVisible() const {
    return visibile_;
}

}
}