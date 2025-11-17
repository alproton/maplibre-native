# Swappy Frame Pacing Integration

This directory contains the build configuration for integrating [Android Game SDK's Swappy Frame Pacing](https://developer.android.com/games/sdk/frame-pacing) into MapLibre Native Android.

## Overview

Swappy is built from source as a **static library** using `libc++_static` to avoid conflicts with other native dependencies. The Game SDK source code is kept **outside the MapLibre Native repository** to reduce repository size.

## Directory Structure

```
~/android-gamesdk/gamesdk/           # Game SDK source (external, not in repo)
  └── games-frame-pacing/            # Swappy source code
      ├── common/
      ├── opengl/
      └── vulkan/

~/mln-proton/platform/android/
  ├── third_party/swappy/
  │   ├── CMakeLists.txt             # Build configuration (in repo)
  │   └── README.md                  # This file
  └── MapLibreAndroid/src/main/java/com/google/androidgamesdk/
      ├── SwappyDisplayManager.java  # Java support classes
      ├── ChoreographerCallback.java
      └── GameSdkDeviceInfoJni.java
```

## Initial Setup

### 1. Download Game SDK Source

```bash
cd ~
git clone https://github.com/android/games-samples.git
cd games-samples/agdk
# The gamesdk directory should be at ~/games-samples/agdk/agde/
```

**Note:** If you cloned it to a different location, create a symbolic link:
```bash
mkdir -p ~/android-gamesdk
ln -s ~/games-samples/agdk/agde ~/android-gamesdk/gamesdk
```

Or set the `GAMESDK_DIR` environment variable:
```bash
export GAMESDK_DIR=/path/to/your/gamesdk
```

### 2. Verify Directory Structure

Ensure the following files exist:
- `~/android-gamesdk/gamesdk/include/swappy/swappyGL.h`
- `~/android-gamesdk/gamesdk/games-frame-pacing/common/SwappyCommon.cpp`

### 3. Build MapLibre Native

```bash
cd ~/mln-proton/platform/android
./gradlew :MapLibreAndroid:assembleDrawableRelease
```

## How It Works

### Build Process

1. **CMake Configuration** (`CMakeLists.txt`):
   - Locates Game SDK source at `~/android-gamesdk/gamesdk` (or `$GAMESDK_DIR`)
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

### Updating Game SDK

```bash
cd ~/android-gamesdk/gamesdk
git pull
# Then rebuild MapLibre Native
cd ~/mln-proton/platform/android
./gradlew clean :MapLibreAndroid:assembleDrawableRelease
```

### Troubleshooting

**Build Error: "Cannot find GAMESDK_DIR"**
- Ensure `~/android-gamesdk/gamesdk` exists
- Or set `GAMESDK_DIR` environment variable

**Runtime Error: "jmethodID was NULL"**
- Java classes are missing from the AAR
- Verify `com/google/androidgamesdk/*.java` files exist in MapLibre's source tree
- Clean and rebuild

**Linker Error: "_binary_classes_dex_start undefined"**
- `ANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE` flag is missing
- Check `CMakeLists.txt` for the flag in `CMAKE_CXX_FLAGS`

## References

- [Android Game SDK Documentation](https://developer.android.com/games/sdk)
- [Swappy Frame Pacing Guide](https://developer.android.com/games/sdk/frame-pacing)
- [Game SDK Source Code](https://github.com/android/games-samples/tree/main/agdk)

