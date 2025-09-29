#pragma once

#include <EGL/egl.h>
#include <map>
#include <string>

namespace mbgl {
namespace android {
namespace egl {
std::string query_current_config_attributes();
std::map<std::string, EGLint> query_all_config_attributes(EGLDisplay display, EGLConfig config);
std::string format_config_attributes(const std::map<std::string, EGLint>& attributes);

} // namespace egl
} // namespace android
} // namespace mbgl
