#!/bin/sh
# Installs the system dependencies needed to build Ulti Jarvis from source.
# Everything else (miniaudio, Lowwi, whisper.cpp, the speech model) is fetched
# automatically by CMake at configure time.
set -e

if [ "$(uname)" = "Darwin" ]; then
    command -v brew >/dev/null 2>&1 || {
        echo "Homebrew required: https://brew.sh" >&2
        exit 1
    }
    brew install cmake ninja pkg-config qt boost webrtc-audio-processing
    exit 0
fi

if [ -n "$MSYSTEM" ]; then
    pacman -S --needed --noconfirm \
        mingw-w64-ucrt-x86_64-clang \
        mingw-w64-ucrt-x86_64-cmake \
        mingw-w64-ucrt-x86_64-ninja \
        mingw-w64-ucrt-x86_64-pkgconf \
        mingw-w64-ucrt-x86_64-qt6-base \
        mingw-w64-ucrt-x86_64-qt6-declarative \
        mingw-w64-ucrt-x86_64-qt6-tools \
        mingw-w64-ucrt-x86_64-boost \
        mingw-w64-ucrt-x86_64-webrtc-audio-processing-1 \
        mingw-w64-ucrt-x86_64-onnxruntime \
        mingw-w64-ucrt-x86_64-vulkan-devel \
        mingw-w64-ucrt-x86_64-spirv-headers \
        mingw-w64-x86_64-nsis
    exit 0
fi

if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y \
        build-essential cmake ninja-build pkg-config git \
        qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools \
        libwebrtc-audio-processing-1-dev libboost-filesystem-dev \
        libvulkan-dev glslc spirv-headers
    exit 0
fi

if command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y \
        gcc-c++ cmake ninja-build pkgconf-pkg-config git \
        qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qttools-devel \
        webrtc-audio-processing-devel boost-devel \
        vulkan-loader-devel glslc spirv-headers-devel
    exit 0
fi

if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --needed --noconfirm \
        base-devel cmake ninja pkgconf git \
        qt6-base qt6-declarative qt6-tools \
        webrtc-audio-processing boost \
        vulkan-headers shaderc spirv-headers
    exit 0
fi

echo "Unsupported package manager. Required: CMake >= 3.30, a C++26 compiler," >&2
echo "Qt6 (Core Gui Network Quick QuickControls2 LinguistTools), pkg-config," >&2
echo "webrtc-audio-processing-1, Boost >= 1.64 (filesystem)." >&2
exit 1
