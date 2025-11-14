cmake_minimum_required(VERSION 3.18.1)
project(swappy C CXX)
set(CMAKE_CXX_STANDARD 17)

# Swappy source directories (external to MapLibre Native repo)
# Set GAMESDK_DIR environment variable or it defaults to ~/android-gamesdk
if(DEFINED ENV{GAMESDK_DIR})
    set(GAMESDK_DIR "$ENV{GAMESDK_DIR}")
else()
    set(GAMESDK_DIR "$ENV{HOME}/android-gamesdk/gamesdk")
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

