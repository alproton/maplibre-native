# Swappy Frame Pacing Integration

This directory contains the build configuration for integrating [Android Game SDK's Swappy Frame Pacing](https://developer.android.com/games/sdk/frame-pacing) into MapLibre Native Android.

## Overview

Swappy is built from source as a **static library** using `libc++_static` to avoid conflicts with other native dependencies. The Game SDK source is **automatically downloaded** during the first build via CMake's FetchContent. No manual setup is required.

## Quick Start

Just build -- no extra setup needed:

```bash
cd ~/mln-proton/platform/android
./gradlew :MapLibreAndroid:assembleDrawableRelease
```

On the first build, CMake will automatically fetch the Android Game SDK source from
`https://android.googlesource.com/platform/frameworks/opt/gamesdk` into the build
directory. Subsequent builds reuse the cached download.

## Advanced: Using a Local Game SDK Checkout

If you prefer to use a local checkout (e.g., for offline builds or custom patches),
set the `GAMESDK_DIR` environment variable:

```bash
export GAMESDK_DIR=/path/to/your/gamesdk
```

This skips the auto-download and uses your local copy instead. The directory must
contain `games-frame-pacing/`, `include/swappy/`, and `src/common/`.

For CI caching, you can also use CMake's built-in override:

```bash
-DFETCHCONTENT_SOURCE_DIR_GAMESDK=/path/to/cached/gamesdk
```

## Directory Structure

```
<build-dir>/_deps/gamesdk-src/      # Auto-fetched Game SDK (in build dir, not in repo)
  ├── include/swappy/               # Public headers
  ├── games-frame-pacing/           # Swappy source code
  │   ├── common/
  │   ├── opengl/
  │   └── vulkan/
  └── src/common/                   # Shared utilities

platform/android/
  ├── third_party/swappy/
  │   ├── CMakeLists.txt            # Build configuration (in repo)
  │   └── README.md                 # This file
  └── MapLibreAndroid/src/main/java/com/google/androidgamesdk/
      ├── SwappyDisplayManager.java # Java support classes
      ├── ChoreographerCallback.java
      └── GameSdkDeviceInfoJni.java
```

## How It Works

### Build Process

1. **CMake Configuration** (`CMakeLists.txt`):
   - Checks for `GAMESDK_DIR` env var (manual override)
   - If not set, auto-fetches the Game SDK via FetchContent (pinned to a known-good commit)
   - Compiles Swappy C++ sources into `libswappy_static.a`
   - Uses `-DANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE` to skip embedded DEX

2. **Java Classes**:
   - Three Java support classes are included directly in MapLibre's source
   - Compiled into the AAR's `classes.jar`
   - Available at runtime via standard Android classpath

3. **Static Linking**:
   - Built with `libc++_static` to avoid `libc++_shared.so` conflicts
   - Links against MapLibre's native library at build time
   - No separate `.so` file needed at runtime

### ANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE Explained

**The Problem:**
The Game SDK can provide Java classes in two ways:

1. **Embedded DEX (default)**: Compile Java classes into a DEX file, embed it as binary data in the `.so`, extract and load at runtime
   - Requires linker symbols: `_binary_classes_dex_start` and `_binary_classes_dex_end`
   - Complex setup requiring custom build process

2. **Classpath (our approach)**: Include Java classes normally in the APK/AAR
   - Standard Android convention
   - Cleaner integration for libraries

**The Solution:**
By defining `ANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE`:
- Game SDK skips the embedded DEX approach
- Uses standard Android classpath instead
- We provide the Java classes directly in MapLibre's source tree
- No special linker symbols required

This is the **recommended approach** for library integration.

## Build Configuration

### NDK Version
- **Required:** NDK 26.1.10909125 or later
- **Why:** Provides C++20 support for MapLibre Native while being compatible with Game SDK

Set in `buildSrc/src/main/kotlin/Versions.kt`:
```kotlin
const val ndkVersion = "26.1.10909125"
```

### C++ Standard Library
- **Static linking:** `-DANDROID_STL=c++_static`
- **Why:** Avoids conflicts with other dependencies using `libc++_shared.so`

### Compiler Flags
```cmake
-std=c++17                              # C++17 for Swappy
-fno-exceptions -fno-rtti               # Reduce binary size
-ffunction-sections -fdata-sections     # Enable unused code elimination
-DANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE  # Skip embedded DEX approach
```

## Maintenance

### Updating the Pinned Game SDK Version

The Game SDK is pinned to a specific commit in `CMakeLists.txt` (the `GIT_TAG` in the
`FetchContent_Declare` call). To update:

1. Find the desired commit hash from
   https://android.googlesource.com/platform/frameworks/opt/gamesdk/+log/refs/heads/main
2. Update the `GIT_TAG` value in `CMakeLists.txt`
3. Clean and rebuild:
   ```bash
   ./gradlew clean :MapLibreAndroid:assembleDrawableRelease
   ```

### Troubleshooting

**Build Error: "Game SDK not found at ..."**
- If using `GAMESDK_DIR`, verify the path contains `games-frame-pacing/`
- If auto-downloading, check your network connection and try a clean build

**Runtime Error: "jmethodID was NULL"**
- Java classes are missing from the AAR
- Verify `com/google/androidgamesdk/*.java` files exist in MapLibre's source tree
- Clean and rebuild

**Linker Error: "_binary_classes_dex_start undefined"**
- `ANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE` flag is missing
- Check `CMakeLists.txt` for the flag in `CMAKE_CXX_FLAGS`

**Slow first build**
- The initial FetchContent clone is a one-time cost (~10-20 MB shallow clone)
- Subsequent builds reuse the cached source in `<build-dir>/_deps/gamesdk-src/`
- For CI, pre-populate the cache with `-DFETCHCONTENT_SOURCE_DIR_GAMESDK=...`

## References

- [Android Game SDK Documentation](https://developer.android.com/games/sdk)
- [Swappy Frame Pacing Guide](https://developer.android.com/games/sdk/frame-pacing)
- [Game SDK Source Code](https://github.com/android/games-samples/tree/main/agdk)

