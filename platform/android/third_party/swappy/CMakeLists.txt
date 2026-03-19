cmake_minimum_required(VERSION 3.18.1)
project(swappy C CXX)
set(CMAKE_CXX_STANDARD 17)

# Resolve the Android Game SDK source directory.
#
# Priority order:
#   1. GAMESDK_DIR environment variable (explicit override)
#   2. GAMESDK_DIR CMake variable (e.g. -DGAMESDK_DIR=... or FETCHCONTENT_SOURCE_DIR_GAMESDK)
#   3. Auto-download via FetchContent from googlesource (default for fresh builds)
if(DEFINED ENV{GAMESDK_DIR})
    set(GAMESDK_DIR "$ENV{GAMESDK_DIR}")
elseif(NOT DEFINED GAMESDK_DIR)
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
    endif()
    set(GAMESDK_DIR "${gamesdk_SOURCE_DIR}")
    message(STATUS "Swappy: auto-fetched Game SDK to ${GAMESDK_DIR}")
endif()

if(NOT EXISTS "${GAMESDK_DIR}/games-frame-pacing")
    message(FATAL_ERROR
        "Game SDK not found at ${GAMESDK_DIR}\n"
        "Either:\n"
        "  - Let the build auto-download it (remove GAMESDK_DIR override), or\n"
        "  - Set GAMESDK_DIR env var to a valid Game SDK checkout containing games-frame-pacing/")
endif()

set(SWAPPY_DIR "${GAMESDK_DIR}/games-frame-pacing")

# Compiler flags from original Swappy build (relaxed for compatibility)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wno-error")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
# Skip embedded DEX linkage (requires Java classes to be available separately)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DANDROIDGAMESDK_NO_BINARY_DEX_LINKAGE")

# Include directories
include_directories(${GAMESDK_DIR}/include)
include_directories(${GAMESDK_DIR}/src/common)
include_directories(${SWAPPY_DIR}/common)
include_directories(${SWAPPY_DIR}/opengl)
include_directories(${SWAPPY_DIR}/vulkan)

# Build swappy_static library
add_library(swappy_static STATIC
    # Common source files
    ${SWAPPY_DIR}/common/ChoreographerFilter.cpp
    ${SWAPPY_DIR}/common/ChoreographerThread.cpp
    ${SWAPPY_DIR}/common/CpuInfo.cpp
    ${SWAPPY_DIR}/common/Settings.cpp
    ${SWAPPY_DIR}/common/Thread.cpp
    ${SWAPPY_DIR}/common/SwappyCommon.cpp
    ${SWAPPY_DIR}/common/swappy_c.cpp
    ${SWAPPY_DIR}/common/SwappyDisplayManager.cpp
    ${SWAPPY_DIR}/common/CPUTracer.cpp
    ${SWAPPY_DIR}/common/FrameStatistics.cpp
    
    # OpenGL source files
    ${SWAPPY_DIR}/opengl/EGL.cpp
    ${SWAPPY_DIR}/opengl/swappyGL_c.cpp
    ${SWAPPY_DIR}/opengl/SwappyGL.cpp
    ${SWAPPY_DIR}/opengl/FrameStatisticsGL.cpp
    
    # Vulkan source files
    ${SWAPPY_DIR}/vulkan/swappyVk_c.cpp
    ${SWAPPY_DIR}/vulkan/SwappyVk.cpp
    ${SWAPPY_DIR}/vulkan/SwappyVkBase.cpp
    ${SWAPPY_DIR}/vulkan/SwappyVkFallback.cpp
    ${SWAPPY_DIR}/vulkan/SwappyVkGoogleDisplayTiming.cpp
    
    # System utils
    ${GAMESDK_DIR}/src/common/system_utils.cpp
)

# Link libraries
target_link_libraries(swappy_static
    android
    log
    GLESv2
    EGL
)

# Export include directories
target_include_directories(swappy_static PUBLIC
    ${GAMESDK_DIR}/include
)

