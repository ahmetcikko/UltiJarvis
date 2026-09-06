#!/bin/sh
# Installs the system dependencies needed to build Ulti Jarvis from source.
# Everything else (miniaudio, Lowwi, whisper.cpp, the speech model) is fetched
# automatically by CMake at configure time.
set -e

WEBRTC_REPO=https://gitlab.freedesktop.org/pulseaudio/webrtc-audio-processing.git
WEBRTC_TAG=v1.3

require_apm() {
    if pkg-config --exists webrtc-audio-processing-1; then
        return 0
    fi
    echo "pkg-config cannot find webrtc-audio-processing-1." >&2
    echo "Ulti Jarvis needs the 1.x API. Packages named plainly" >&2
    echo "'webrtc-audio-processing' are often 0.3 or 2.x and will not work." >&2
    exit 1
}

pick() {
    for _p in "$@"; do
        if $PICK_QUERY "$_p" >/dev/null 2>&1; then
            printf '%s' "$_p"
            return 0
        fi
    done
    return 1
}

if [ "$(uname)" = "Darwin" ]; then
    command -v brew >/dev/null 2>&1 || {
        echo "Homebrew required: https://brew.sh" >&2
        exit 1
    }
    brew install cmake ninja pkgconf qt boost abseil meson

    # webrtc-audio-processing has no Homebrew formula. Build the upstream
    # release into the Homebrew prefix, which pkg-config already searches.
    # abseil comes from Homebrew so the build does not vendor its own copy
    # and overwrite the headers already installed there.
    if ! pkg-config --exists webrtc-audio-processing-1; then
        work=$(mktemp -d)
        trap 'rm -rf "$work"' EXIT
        git clone --depth 1 --branch "$WEBRTC_TAG" "$WEBRTC_REPO" "$work/src"
        meson setup "$work/build" "$work/src" \
            --prefix "$(brew --prefix)" \
            --buildtype release
        meson install -C "$work/build"
    fi
    require_apm
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
    require_apm
    exit 0
fi

if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y \
        build-essential cmake ninja-build pkg-config git \
        qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools \
        libboost-filesystem-dev \
        libvulkan-dev glslc spirv-headers
    # Debian ships 1.x as libwebrtc-audio-processing-dev from trixie onward;
    # Ubuntu keeps 0.3 under that name and ships 1.x with the -1- infix.
    PICK_QUERY="apt-cache show"
    apm=$(pick libwebrtc-audio-processing-1-dev libwebrtc-audio-processing-dev) || apm=
    if [ -n "$apm" ]; then
        sudo apt-get install -y "$apm"
    fi
    require_apm
    exit 0
fi

if command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y \
        gcc-c++ cmake ninja-build pkgconf-pkg-config git \
        qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qttools-devel \
        boost-devel \
        vulkan-loader-devel glslc spirv-headers-devel
    PICK_QUERY="dnf info"
    apm=$(pick webrtc-audio-processing-1-devel webrtc-audio-processing-devel) || apm=
    if [ -n "$apm" ]; then
        sudo dnf install -y "$apm"
    fi
    require_apm
    exit 0
fi

if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --needed --noconfirm \
        base-devel cmake ninja pkgconf git \
        qt6-base qt6-declarative qt6-tools \
        webrtc-audio-processing-1 boost \
        vulkan-headers shaderc spirv-headers
    require_apm
    exit 0
fi

echo "Unsupported package manager. Required: CMake >= 3.30, a C++26 compiler," >&2
echo "Qt6 (Core Gui Network Quick QuickControls2 LinguistTools), pkg-config," >&2
echo "webrtc-audio-processing-1, Boost >= 1.64 (filesystem)." >&2
exit 1
