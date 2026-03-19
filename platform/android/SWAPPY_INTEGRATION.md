# Swappy Frame Pacing Integration - Summary

## Overview

This document summarizes the integration of Android Game SDK's Swappy Frame Pacing library into MapLibre Native Android, configured to use **static C++ library** linking to avoid conflicts with other dependencies.

## Problem Statement

MapLibre Native and `android-spatialite` both included `libc++_shared.so`, causing build conflicts:
```
2 files found with path 'lib/arm64-v8a/libc++_shared.so'
  - android-spatialite-2.1.1-alpha/jni/arm64-v8a/libc++_shared.so
  - MapLibreAndroid-drawable-release/jni/arm64-v8a/libc++_shared.so
```

## Solution

1. **Static C++ Linking**: Build MapLibre Native with `libc++_static` instead of `libc++_shared`
2. **Swappy from Source**: Build Swappy statically from Game SDK source (not available as prebuilt static library)
3. **Auto-Download via FetchContent**: Game SDK source is automatically fetched during the first build -- no manual setup required

## Architecture Changes

### Repository Size Impact
- **Before:** `third_party/` = 252 MB (with embedded Game SDK source)
- **After:** `third_party/` = 12 KB (build config only)
- **Reduction:** 99.995% smaller

### Build Configuration

**Source:** Auto-fetched into `<build-dir>/_deps/gamesdk-src/` via CMake FetchContent
(or manual override via `GAMESDK_DIR` env var)

**Build Flow:**
```
CMake Configure
    ↓
FetchContent auto-downloads Game SDK (if GAMESDK_DIR not set)
    ↓
Patch ChoreographerShim.h for NDK 26+ compatibility
    ↓
Swappy CMake Build
    ↓
libswappy_static.a
    ↓
Link with MapLibre Native
    ↓
libmaplibre.so (single .so with static C++)
```

## CMake Build System In Depth

The Swappy build is defined in `platform/android/third_party/swappy/CMakeLists.txt`. This
file is the sole entry point for Swappy in the build graph, included only by
`platform/android/MapLibreAndroid/src/cpp/CMakeLists.txt` when `MLN_WITH_OPENGL` is ON
(the drawable and legacy flavors). It is never triggered by non-Android platforms.

### Game SDK Source Resolution

The CMake file resolves the Game SDK source directory using a three-level priority:

```cmake
# Priority 1: Environment variable (explicit override)
if(DEFINED ENV{GAMESDK_DIR})
    set(GAMESDK_DIR "$ENV{GAMESDK_DIR}")

# Priority 2: CMake variable (e.g. -DGAMESDK_DIR=... or FETCHCONTENT_SOURCE_DIR_GAMESDK)
elseif(NOT DEFINED GAMESDK_DIR)

    # Priority 3: Auto-download from googlesource via FetchContent
    include(FetchContent)
    FetchContent_Declare(
        gamesdk
        GIT_REPOSITORY https://android.googlesource.com/platform/frameworks/opt/gamesdk
        GIT_TAG 044fd03c4a7d3b75aeb6ca2bd7fb6155d2cdb787
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_GetProperties(gamesdk)
    if(NOT gamesdk_POPULATED)
        FetchContent_Populate(gamesdk)
        # ... patch ChoreographerShim.h (see below) ...
    endif()
    set(GAMESDK_DIR "${gamesdk_SOURCE_DIR}")
endif()
```

**Why `FetchContent_Populate` instead of `FetchContent_MakeAvailable`:**
The Game SDK repo contains `build.gradle` and `settings.gradle` at its root. If the repo
ever adds a `CMakeLists.txt`, `FetchContent_MakeAvailable` would automatically call
`add_subdirectory()` on the entire Game SDK project, pulling in unrelated build targets.
Using `FetchContent_Populate` explicitly downloads the source without attempting to
configure or build anything from it. We then manually reference only the specific source
files we need.

**Why the canonical googlesource repo:**
The Game SDK source lives at `https://android.googlesource.com/platform/frameworks/opt/gamesdk`.
This is the authoritative upstream (maintained by Google's AGDK team), as opposed to the
`github.com/android/games-samples` mirror which reorganizes the source under `agdk/agde/`.
The googlesource repo has the standard layout that our CMakeLists.txt expects:
`games-frame-pacing/`, `include/swappy/`, and `src/common/`.

**Why `GIT_SHALLOW TRUE`:**
The full Game SDK repo history is large. Shallow clone fetches only the single commit we
reference, reducing the download from hundreds of MB to ~10-20 MB.

**Pinning to a specific commit:**
The `GIT_TAG` is set to a specific commit hash (`044fd03c...`) rather than a branch name
like `main`. This ensures reproducible builds -- every developer and CI system builds
against exactly the same Game SDK source. To update, replace the hash and clean rebuild.

### ChoreographerShim.h Patch (NDK 26+ Compatibility)

After FetchContent downloads the Game SDK source, the build automatically patches
`src/common/ChoreographerShim.h` to fix a compilation error with NDK 26+.

**The Problem:**

The Game SDK's `ChoreographerShim.h` was written for older NDKs where Choreographer APIs
above the target API level were not declared in the NDK headers. It provides fallback
typedefs guarded by `#if __ANDROID_API__ < 33`:

```c
// In the upstream (unpatched) ChoreographerShim.h:
#if __ANDROID_API__ < 33

// These are typedef'd as function POINTER types, intended for use with dlsym():
typedef void (*AChoreographer_postVsyncCallback)(...);
typedef size_t (*AChoreographerFrameCallbackData_getPreferredFrameTimelineIndex)(...);
typedef int64_t (*AChoreographerFrameCallbackData_getFrameTimelineExpectedPresentationTimeNanos)(...);
typedef int64_t (*AChoreographerFrameCallbackData_getFrameTimelineDeadlineNanos)(...);

#endif
```

Starting with NDK r26, the NDK headers declare ALL Choreographer APIs unconditionally
using `__attribute__((availability(...)))` annotations (defined via the `__INTRODUCED_IN`
macro in `<android/versioning.h>`). This means `AChoreographer_postVsyncCallback` etc. are
now declared as **actual function declarations** in `<android/choreographer.h>`, regardless
of the target API level:

```c
// In NDK 26's choreographer.h -- always present regardless of __ANDROID_API__:
void AChoreographer_postVsyncCallback(AChoreographer* choreographer,
                                        AChoreographer_vsyncCallback callback, void* data)
        __INTRODUCED_IN(33);
```

When `__ANDROID_API__ < 33` (e.g., targeting API 21), both declarations are visible:
the NDK's **function declaration** and the Game SDK's **typedef with the same name**. The
compiler sees this as "redefinition of 'AChoreographer_postVsyncCallback' as different kind
of symbol" and emits a fatal error.

**The Fix:**

The build patches the guard to also check `__NDK_MAJOR__` (defined in
`<android/ndk-version.h>`, available in all NDK versions):

```cmake
# In CMakeLists.txt, after FetchContent_Populate:
string(REPLACE
    "#if __ANDROID_API__ < 33"
    "#if __ANDROID_API__ < 33 && (!defined(__NDK_MAJOR__) || __NDK_MAJOR__ < 26)"
    _shim_contents "${_shim_contents}")
```

This changes the guard in `ChoreographerShim.h` to:

```c
#if __ANDROID_API__ < 33 && (!defined(__NDK_MAJOR__) || __NDK_MAJOR__ < 26)
```

**Effect:** On NDK 26+, the entire `#if` block is skipped. The function declarations from
the NDK header are used directly, and the conflicting typedefs are never defined. On older
NDKs (< 26), the behavior is unchanged -- the shim typedefs are still provided as before.

**Why this is done at CMake configure time (not a .patch file):**
Using `file(READ)` + `string(REPLACE)` + `file(WRITE)` in CMake is self-contained -- no
external `patch` or `git apply` dependency required. The patch runs once (guarded by
`if(NOT gamesdk_POPULATED)`) and modifies the fetched source in-place. Subsequent
configures reuse the already-patched source.

### Validation After Build

The build automatically validates that `GAMESDK_DIR` points to a valid Game SDK checkout:

```cmake
if(NOT EXISTS "${GAMESDK_DIR}/games-frame-pacing")
    message(FATAL_ERROR
        "Game SDK not found at ${GAMESDK_DIR}\n"
        "Either:\n"
        "  - Let the build auto-download it (remove GAMESDK_DIR override), or\n"
        "  - Set GAMESDK_DIR env var to a valid Game SDK checkout containing games-frame-pacing/")
endif()
```

This catches misconfigured `GAMESDK_DIR` overrides early with a clear error message.

### Source Files Compiled

The `swappy_static` library is built from these Game SDK source files:

| Category | Files | Purpose |
|----------|-------|---------|
| **Common** | `ChoreographerFilter.cpp`, `ChoreographerThread.cpp`, `CpuInfo.cpp`, `Settings.cpp`, `Thread.cpp`, `SwappyCommon.cpp`, `swappy_c.cpp`, `SwappyDisplayManager.cpp`, `CPUTracer.cpp`, `FrameStatistics.cpp` | Core frame pacing logic, choreographer integration, frame statistics |
| **OpenGL** | `EGL.cpp`, `swappyGL_c.cpp`, `SwappyGL.cpp`, `FrameStatisticsGL.cpp` | OpenGL ES swap chain management, EGL buffer control |
| **Vulkan** | `swappyVk_c.cpp`, `SwappyVk.cpp`, `SwappyVkBase.cpp`, `SwappyVkFallback.cpp`, `SwappyVkGoogleDisplayTiming.cpp` | Vulkan present timing (compiled but only Vulkan entry points are unused in our OpenGL build) |
| **Utils** | `system_utils.cpp` | Platform detection and system property access |

### Linking

The `swappy_static` library links against:
- `android` -- Native Android APIs (Choreographer, Looper)
- `log` -- Android logging
- `GLESv2` -- OpenGL ES 2.0
- `EGL` -- EGL display/surface management

It is then linked into the main `maplibre` shared library:
```cmake
# In MapLibreAndroid/src/cpp/CMakeLists.txt:
add_subdirectory(../../../third_party/swappy ${CMAKE_CURRENT_BINARY_DIR}/swappy)
target_link_libraries(maplibre PRIVATE swappy_static)
```

## Other Technical Details

### ANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE

**What it does:**
- Disables the Game SDK's embedded DEX approach
- Allows using standard Android classpath for Java classes

**Why we need it:**
The Game SDK supports two methods for providing Java support classes:

| Approach | Description | Pros | Cons |
|----------|-------------|------|------|
| **Embedded DEX** (default) | Compile Java -> DEX -> embed binary in `.so` -> extract at runtime | Self-contained | Complex, requires linker symbols |
| **Classpath** (our choice) | Include Java classes in AAR normally | Simple, standard Android | Requires Java sources in library |

**Implementation:**
```cmake
# In CMakeLists.txt
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE")
```

```java
// Added to MapLibre source tree
com/google/androidgamesdk/
├── SwappyDisplayManager.java      # Display refresh rate management
├── ChoreographerCallback.java     # Frame callback handling
└── GameSdkDeviceInfoJni.java      # Device info (optional)
```

### Static Library Configuration

**build.gradle.kts:**
```kotlin
arguments("-DANDROID_STL=c++_static")  // All build flavors
// Removed: implementation(libs.gamesFramePacing)
// Removed: prefab = true
```

### NDK Compatibility

**NDK Version:** 26.1.10909125

**Why this version:**
- Provides C++20 support for MapLibre Native (`<numbers>`, `std::ranges`)
- Compatible with Game SDK source code
- Native Choreographer API support (reduces shim complexity)

### Compiler Flags

```cmake
-std=c++17                              # Swappy requires C++17
-fno-exceptions -fno-rtti               # Match MapLibre configuration
-ffunction-sections -fdata-sections     # Enable linker garbage collection
-DANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE  # Use classpath for Java classes
```

## File Changes Summary

### Modified Files

| File | Change | Reason |
|------|--------|--------|
| `MapLibreAndroid/build.gradle.kts` | Set `-DANDROID_STL=c++_static` | Use static C++ library |
| `MapLibreAndroid/src/cpp/CMakeLists.txt` | Add Swappy subdirectory | Build Swappy from source |
| `buildSrc/src/main/kotlin/Versions.kt` | NDK -> 26.1.10909125 | C++20 support + compatibility |

### Added Files

| File | Purpose |
|------|---------|
| `third_party/swappy/CMakeLists.txt` | Build config: auto-fetch, patch, compile Swappy from source |
| `third_party/swappy/README.md` | Quick-start documentation |
| `MapLibreAndroid/src/main/java/com/google/androidgamesdk/*.java` | Java support classes (3 files) |
| `SWAPPY_INTEGRATION.md` | This document |

## Verification

### Build Artifacts

**AAR Contents:**
```
MapLibreAndroid-drawable-release.aar
├── classes.jar
│   └── com/google/androidgamesdk/     ← Java classes included
│       ├── SwappyDisplayManager.class
│       ├── ChoreographerCallback.class
│       └── GameSdkDeviceInfoJni.class
└── jni/
    ├── arm64-v8a/libmaplibre.so       ← Static C++, Swappy linked in
    ├── armeabi-v7a/libmaplibre.so
    ├── x86/libmaplibre.so
    └── x86_64/libmaplibre.so
    
✅ NO libc++_shared.so
✅ NO libswappy_static.so (linked statically into libmaplibre.so)
```

### Runtime Verification

**Logcat output shows Swappy is active:**
```
SwappyDisplayManager: Starting looper thread
ChoreographerCallback: Starting looper thread
Swappy: Initialized successfully
```

## Developer Workflow

### Build (no setup required)
```bash
cd ~/mln-proton/platform/android
./gradlew :MapLibreAndroid:assembleDrawableRelease

# The build automatically:
# 1. Fetches Game SDK via FetchContent (first build only)
# 2. Compiles Swappy from source
# 3. Links statically with MapLibre
# 4. Packages Java classes into AAR
```

### Using a local Game SDK (optional)
```bash
export GAMESDK_DIR=/path/to/your/gamesdk
./gradlew :MapLibreAndroid:assembleDrawableRelease
```

### CI caching
```bash
# Pre-populate the FetchContent cache to avoid re-downloading
cmake ... -DFETCHCONTENT_SOURCE_DIR_GAMESDK=/path/to/cached/gamesdk
```

### Updating the pinned Game SDK version
Update the `GIT_TAG` commit hash in `third_party/swappy/CMakeLists.txt`, then clean and rebuild.

## Troubleshooting

### Build Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `Game SDK not found at ...` | `GAMESDK_DIR` points to invalid path | Fix the path or unset `GAMESDK_DIR` to use auto-download |
| `_binary_classes_dex_start undefined` | Missing DEX linkage flag | Verify `ANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE` in CMakeLists.txt |
| `<numbers> file not found` | NDK too old | Use NDK 26.1.10909125+ |
| `AChoreographer_postVsyncCallback redefinition` | NDK 26+ vs Game SDK typedef conflict | See "ChoreographerShim.h Patch" section above; if using `GAMESDK_DIR` override, you must patch your local copy manually |
| FetchContent download failure | Network issue or git not available | Check connectivity; or set `GAMESDK_DIR` for offline builds |
| FetchContent clone hangs | Corporate firewall blocking googlesource.com | Clone manually and use `GAMESDK_DIR` or `FETCHCONTENT_SOURCE_DIR_GAMESDK` |

**Note on `GAMESDK_DIR` override and the ChoreographerShim patch:**
The automatic ChoreographerShim.h patch only runs when FetchContent downloads the source
(Priority 3). If you provide your own Game SDK checkout via `GAMESDK_DIR`, you are
responsible for ensuring it is compatible with your NDK version. For NDK 26+, you must
manually apply the same fix to `src/common/ChoreographerShim.h` in your checkout:

```diff
- #if __ANDROID_API__ < 33
+ #if __ANDROID_API__ < 33 && (!defined(__NDK_MAJOR__) || __NDK_MAJOR__ < 26)
```

### Runtime Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `jmethodID was NULL` | Java classes missing | Verify `com/google/androidgamesdk/*.java` in source tree, rebuild |
| `UnsatisfiedLinkError: swappy` | Wrong linking mode | Ensure using static linking, not prefab |
| `libc++_shared.so conflict` | Not using static STL | Verify `-DANDROID_STL=c++_static` in all flavors |

## Performance Impact

**Binary Size:**
- Swappy static library: ~150 KB per architecture
- Net impact: Minimal (< 1% of total AAR size)

**Runtime:**
- Frame pacing improves frame consistency
- No measurable performance overhead
- Reduces jank and improves user experience

## References

- [Android Game SDK](https://developer.android.com/games/sdk)
- [Swappy Frame Pacing](https://developer.android.com/games/sdk/frame-pacing)
- [Game SDK Source (googlesource)](https://android.googlesource.com/platform/frameworks/opt/gamesdk)
- [NDK C++ Library Support](https://developer.android.com/ndk/guides/cpp-support)
- [Static vs Shared STL](https://developer.android.com/ndk/guides/cpp-support#static_runtimes)

