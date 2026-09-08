#!/bin/sh

set -e

WEBRTC_REPO=https://gitlab.freedesktop.org/pulseaudio/webrtc-audio-processing.git
WEBRTC_TAG=v1.3

have_apm() {
    pkg-config --exists webrtc-audio-processing-1
}

build_webrtc() {
    prefix=$1
    as_root=$2
    command -v meson >/dev/null 2>&1 || {
        echo "meson is required to build webrtc-audio-processing." >&2
        exit 1
    }
    work=$(mktemp -d)
    trap 'rm -rf "$work"' EXIT
    git clone --depth 1 --branch "$WEBRTC_TAG" "$WEBRTC_REPO" "$work/src"
    meson setup "$work/build" "$work/src" \
        --prefix "$prefix" \
        --libdir lib \
        --buildtype release
    $as_root meson install -C "$work/build"
}

require_apm() {
    if have_apm; then
        return 0
    fi
    echo "pkg-config still cannot find webrtc-audio-processing-1 after install." >&2
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

    if ! have_apm; then
        build_webrtc "$(brew --prefix)" ""
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
        mingw-w64-ucrt-x86_64-qt6-svg \
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
        build-essential cmake ninja-build pkg-config git meson \
        qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools \
        libboost-filesystem-dev \
        libvulkan-dev glslc spirv-headers \
        rpm

    PICK_QUERY="apt-cache show"
    apm=$(pick libwebrtc-audio-processing-1-dev libwebrtc-audio-processing-dev) || apm=
    if [ -n "$apm" ]; then
        sudo apt-get install -y "$apm"
    fi
    svg=$(pick libqt6svg6-dev qt6-svg-dev libqt6svg6) || svg=
    if [ -n "$svg" ]; then
        sudo apt-get install -y "$svg"
    fi
    if ! have_apm; then
        build_webrtc /usr/local sudo
        sudo ldconfig
    fi
    require_apm
    exit 0
fi

if command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y \
        gcc-c++ cmake ninja-build pkgconf-pkg-config git meson \
        qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qttools-devel \
        boost-devel \
        vulkan-loader-devel glslc spirv-headers-devel \
        rpm-build dpkg
    PICK_QUERY="dnf info"
    apm=$(pick webrtc-audio-processing-1-devel webrtc-audio-processing-devel) || apm=
    if [ -n "$apm" ]; then
        sudo dnf install -y "$apm"
    fi
    svg=$(pick qt6-qtsvg-devel qt6-qtsvg) || svg=
    if [ -n "$svg" ]; then
        sudo dnf install -y "$svg"
    fi
    if ! have_apm; then
        build_webrtc /usr/local sudo
        sudo ldconfig
    fi
    require_apm
    exit 0
fi

if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --needed --noconfirm \
        base-devel cmake ninja pkgconf git meson \
        qt6-base qt6-declarative qt6-tools qt6-svg \
        boost \
        vulkan-headers shaderc spirv-headers
    PICK_QUERY="pacman -Si"
    apm=$(pick webrtc-audio-processing-1) || apm=
    if [ -n "$apm" ]; then
        sudo pacman -S --needed --noconfirm "$apm"
    fi
    if ! have_apm; then
        build_webrtc /usr/local sudo
        sudo ldconfig
    fi
    require_apm
    exit 0
fi

echo "Unsupported package manager. Required: CMake >= 3.30, a C++26 compiler," >&2
echo "Qt6 (Core Gui Network Quick QuickControls2 LinguistTools), pkg-config," >&2
echo "webrtc-audio-processing-1, Boost >= 1.64 (filesystem)." >&2
exit 1
