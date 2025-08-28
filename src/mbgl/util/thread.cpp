#include <functional>
#include <mbgl/platform/settings.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/platform.hpp>
#include <mbgl/util/thread.hpp>
#include <stdexcept>

namespace mbgl {
namespace util {

std::function<void()> makeThreadPrioritySetter(std::string threadType_) {
    return [threadType = std::move(threadType_)] {
        auto& settings = platform::Settings::getInstance();
        auto value = settings.get(threadType);
        if (auto* priority = value.getDouble()) {
            platform::setCurrentThreadPriority(*priority);
        } else {
            platform::makeThreadLowPriority();
        }
    };
}

ThreadPriorityOverrider::ThreadPriorityOverride ThreadPriorityOverrider::threadPriorityOverride =
    ThreadPriorityOverrider::ThreadPriorityOverride::NONE;

ThreadPriorityOverrider::ThreadPriorityOverrider(int threadPriorityOverride_) {
    switch (threadPriorityOverride_) {
        case static_cast<int>(ThreadPriorityOverride::NONE):
            threadPriorityOverride = ThreadPriorityOverride::NONE;
            break;
        case static_cast<int>(ThreadPriorityOverride::FORCE_LOW):
            Log::Warning(Event::General, "Forcing low thread priority");
            threadPriorityOverride = ThreadPriorityOverride::FORCE_LOW;
            break;
        case static_cast<int>(ThreadPriorityOverride::FORCE_HIGH):
            Log::Warning(Event::General, "Forcing high thread priority");
            threadPriorityOverride = ThreadPriorityOverride::FORCE_HIGH;
            break;
        default:
            throw std::runtime_error("Invalid thread priority override value");
    }
}

} // namespace util
} // namespace mbgl
