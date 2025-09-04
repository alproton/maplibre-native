#include "android_egl_helper.hpp"

#include <map>
#include <string>
#include <vector>
#include <sstream>

#ifdef EGL_USE_X11
#include <X11/Xutil.h> // For X11 Visual types like TrueColor, etc.
#endif

namespace mbgl {
namespace android {
namespace egl {
// --- Helper functions to convert EGL constants to strings ---
std::string caveat_to_string(EGLint value) {
    switch (value) {
        case EGL_NONE:
            return "EGL_NONE";
        case EGL_SLOW_CONFIG:
            return "EGL_SLOW_CONFIG";
        case EGL_NON_CONFORMANT_CONFIG:
            return "EGL_NON_CONFORMANT_CONFIG";
        default:
            return "Unknown";
    }
}

std::string surface_type_to_string(EGLint value) {
    std::stringstream ss;
    if (value & EGL_WINDOW_BIT) ss << "EGL_WINDOW_BIT ";
    if (value & EGL_PBUFFER_BIT) ss << "EGL_PBUFFER_BIT ";
    if (value & EGL_PIXMAP_BIT) ss << "EGL_PIXMAP_BIT ";
    std::string result = ss.str();
    return result.empty() ? "None" : result;
}

std::string conformant_to_string(EGLint value) {
    std::stringstream ss;
    if (value & EGL_OPENGL_BIT) ss << "EGL_OPENGL_BIT ";
    if (value & EGL_OPENGL_ES_BIT) ss << "EGL_OPENGL_ES_BIT ";
    if (value & EGL_OPENGL_ES2_BIT) ss << "EGL_OPENGL_ES2_BIT ";
    if (value & EGL_OPENGL_ES3_BIT) ss << "EGL_OPENGL_ES3_BIT ";
    if (value & EGL_OPENVG_BIT) ss << "EGL_OPENVG_BIT ";
    std::string result = ss.str();
    return result.empty() ? "None" : result;
}

std::string transparent_type_to_string(EGLint value) {
    switch (value) {
        case EGL_NONE:
            return "EGL_NONE";
        case EGL_TRANSPARENT_RGB:
            return "EGL_TRANSPARENT_RGB";
        default:
            return "Unknown";
    }
}

/**
 * @brief Converts EGL_NATIVE_VISUAL_TYPE to a descriptive string.
 *
 * The values are platform-specific. This function provides translations
 * for the X11 windowing system and a fallback for others.
 */
std::string native_visual_type_to_string(EGLint value) {
#ifdef EGL_USE_X11
    // On X11, the value maps to standard X Visual classes
    switch (value) {
        case TrueColor:
            return "TrueColor";
        case DirectColor:
            return "DirectColor";
        case StaticColor:
            return "StaticColor";
        case StaticGray:
            return "StaticGray";
        case GrayScale:
            return "GrayScale";
        case PseudoColor:
            return "PseudoColor";
        default:
            return "Unknown X11 Visual Class";
    }
#else
    // On other platforms (Android, Windows, etc.), the value is implementation-defined
    if (value == EGL_NONE) {
        return "EGL_NONE";
    }
    // Return the integer value as a string since we can't translate it
    return "Platform-specific value: " + std::to_string(value);
#endif
}

/**
 * @brief Main function to find and format current EGL config attributes as a string.
 */
std::string query_current_config_attributes() {
    // 1. Get the current display for this thread
    EGLDisplay display = eglGetCurrentDisplay();
    if (display == EGL_NO_DISPLAY) {
        return "Error: No current EGL display found for this thread.";
    }

    // 2. Get the current drawing surface for this thread
    EGLSurface surface = eglGetCurrentSurface(EGL_DRAW);
    if (surface == EGL_NO_SURFACE) {
        return "Error: No current EGL surface found for this thread.";
    }

    // 3. From the surface, query the ID of the EGLConfig that created it
    EGLint current_config_id;
    if (!eglQuerySurface(display, surface, EGL_CONFIG_ID, &current_config_id)) {
        return "Error: Failed to query EGL_CONFIG_ID from the current surface.";
    }

    // 4. Find the EGLConfig handle that matches the retrieved ID
    EGLint num_configs;
    eglGetConfigs(display, nullptr, 0, &num_configs);
    if (num_configs == 0) {
        return "Error: No EGL configs found for the current display.";
    }
    std::vector<EGLConfig> configs(num_configs);
    eglGetConfigs(display, configs.data(), num_configs, &num_configs);

    EGLConfig matched_config = nullptr;
    for (EGLConfig config_iter : configs) {
        EGLint id_from_list;
        eglGetConfigAttrib(display, config_iter, EGL_CONFIG_ID, &id_from_list);
        if (id_from_list == current_config_id) {
            matched_config = config_iter;
            break;
        }
    }

    if (matched_config == nullptr) {
        return "Error: Could not find an EGLConfig matching the current surface's config ID.";
    }

    // 5. Query the attributes as a map
    std::map<std::string, EGLint> attributes_map = query_all_config_attributes(display, matched_config);

    // 6. Format the map into a descriptive string and return it
    return format_config_attributes(attributes_map);
}

/**
 * @brief Formats a map of EGL attributes into a single descriptive string.
 */
std::string format_config_attributes(const std::map<std::string, EGLint>& attributes) {
    std::stringstream ss;
    ss << "--- Attributes of Current EGLConfig ---\n";

    for (const auto& pair : attributes) {
        ss << pair.first << ": ";
        if (pair.first == "EGL_CONFIG_CAVEAT") {
            ss << caveat_to_string(pair.second);
        } else if (pair.first == "EGL_SURFACE_TYPE") {
            ss << surface_type_to_string(pair.second);
        } else if (pair.first == "EGL_CONFORMANT" || pair.first == "EGL_RENDERABLE_TYPE") {
            ss << conformant_to_string(pair.second);
        } else if (pair.first == "EGL_TRANSPARENT_TYPE") {
            ss << transparent_type_to_string(pair.second);
        } else if (pair.first == "EGL_NATIVE_VISUAL_TYPE") {
            ss << native_visual_type_to_string(pair.second);
        } else {
            ss << pair.second; // Default to printing the integer value
        }
        ss << "\n";
    }
    ss << "---------------------------------------";
    return ss.str();
}

/**
 * @brief (Helper) Queries all EGL 1.4 config attributes and returns a map.
 */
std::map<std::string, EGLint> query_all_config_attributes(EGLDisplay display, EGLConfig config) {
    std::map<std::string, EGLint> attributes;
    if (display == EGL_NO_DISPLAY || config == nullptr) {
        return attributes;
    }
    const std::vector<std::pair<EGLint, std::string>> attribute_keys = {
        {EGL_BUFFER_SIZE, "EGL_BUFFER_SIZE"},
        {EGL_RED_SIZE, "EGL_RED_SIZE"},
        {EGL_GREEN_SIZE, "EGL_GREEN_SIZE"},
        {EGL_BLUE_SIZE, "EGL_BLUE_SIZE"},
        {EGL_ALPHA_SIZE, "EGL_ALPHA_SIZE"},
        {EGL_BIND_TO_TEXTURE_RGB, "EGL_BIND_TO_TEXTURE_RGB"},
        {EGL_BIND_TO_TEXTURE_RGBA, "EGL_BIND_TO_TEXTURE_RGBA"},
        {EGL_CONFIG_CAVEAT, "EGL_CONFIG_CAVEAT"},
        {EGL_CONFIG_ID, "EGL_CONFIG_ID"},
        {EGL_CONFORMANT, "EGL_CONFORMANT"},
        {EGL_DEPTH_SIZE, "EGL_DEPTH_SIZE"},
        {EGL_LEVEL, "EGL_LEVEL"},
        {EGL_MAX_PBUFFER_WIDTH, "EGL_MAX_PBUFFER_WIDTH"},
        {EGL_MAX_PBUFFER_HEIGHT, "EGL_MAX_PBUFFER_HEIGHT"},
        {EGL_MAX_PBUFFER_PIXELS, "EGL_MAX_PBUFFER_PIXELS"},
        {EGL_MAX_SWAP_INTERVAL, "EGL_MAX_SWAP_INTERVAL"},
        {EGL_MIN_SWAP_INTERVAL, "EGL_MIN_SWAP_INTERVAL"},
        {EGL_NATIVE_RENDERABLE, "EGL_NATIVE_RENDERABLE"},
        {EGL_NATIVE_VISUAL_ID, "EGL_NATIVE_VISUAL_ID"},
        {EGL_NATIVE_VISUAL_TYPE, "EGL_NATIVE_VISUAL_TYPE"},
        {EGL_RENDERABLE_TYPE, "EGL_RENDERABLE_TYPE"},
        {EGL_SAMPLE_BUFFERS, "EGL_SAMPLE_BUFFERS"},
        {EGL_SAMPLES, "EGL_SAMPLES"},
        {EGL_STENCIL_SIZE, "EGL_STENCIL_SIZE"},
        {EGL_SURFACE_TYPE, "EGL_SURFACE_TYPE"},
        {EGL_TRANSPARENT_TYPE, "EGL_TRANSPARENT_TYPE"},
        {EGL_TRANSPARENT_RED_VALUE, "EGL_TRANSPARENT_RED_VALUE"},
        {EGL_TRANSPARENT_GREEN_VALUE, "EGL_TRANSPARENT_GREEN_VALUE"},
        {EGL_TRANSPARENT_BLUE_VALUE, "EGL_TRANSPARENT_BLUE_VALUE"}};
    for (const auto& attr_pair : attribute_keys) {
        EGLint value;
        if (eglGetConfigAttrib(display, config, attr_pair.first, &value)) {
            attributes[attr_pair.second] = value;
        }
    }
    return attributes;
}
} // namespace egl
} // namespace android
} // namespace mbgl
