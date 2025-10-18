#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS_DIR="$ROOT_DIR/build/deps"
SRC_DIR="$DEPS_DIR/src"
BUILD_DIR="$ROOT_DIR/build/mac"
INSTALL_DIR="$DEPS_DIR/install"

OPENSSL_VERSION="3.3.1"
GLFW_VERSION="3.4"

CPU_COUNT="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

log() {
    printf '[build_mac] %s\n' "$*"
}

fetch_tarball() {
    local url="$1"
    local output="$2"

    if [[ -f "$output" ]]; then
        return
    fi
    log "Downloading $(basename "$output")"
    mkdir -p "$(dirname "$output")"
    curl -L --fail "$url" -o "$output"
}

extract_tarball() {
    local tarball="$1"
    local dest="$2"
    local marker="$dest/.extracted"

    if [[ -f "$marker" ]]; then
        return
    fi
    log "Extracting $(basename "$tarball")"
    rm -rf "$dest"
    mkdir -p "$dest"
    tar -xf "$tarball" --strip-components=1 -C "$dest"
    touch "$marker"
}

build_openssl() {
    local prefix="$INSTALL_DIR/openssl"
    local build_marker="$prefix/.built"

    if [[ -f "$build_marker" ]]; then
        log "OpenSSL already built, skipping"
        echo "$prefix"
        return
    fi

    local tarball="$SRC_DIR/openssl-$OPENSSL_VERSION.tar.gz"
    local source_dir="$SRC_DIR/openssl-$OPENSSL_VERSION"

    fetch_tarball "https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz" "$tarball"
    extract_tarball "$tarball" "$source_dir"

    log "Building OpenSSL ${OPENSSL_VERSION}"
    pushd "$source_dir" >/dev/null
    ./config --prefix="$prefix" --libdir=lib no-shared
    make -j"$CPU_COUNT"
    make install_sw
    popd >/dev/null

    touch "$build_marker"
    echo "$prefix"
}

build_glfw() {
    local prefix="$INSTALL_DIR/glfw"
    local build_marker="$prefix/.built"

    if [[ -f "$build_marker" ]]; then
        log "GLFW already built, skipping"
        echo "$prefix"
        return
    fi

    local tarball="$SRC_DIR/glfw-$GLFW_VERSION.tar.gz"
    local source_dir="$SRC_DIR/glfw-$GLFW_VERSION"
    local build_subdir="$source_dir/build"

    fetch_tarball "https://github.com/glfw/glfw/archive/refs/tags/${GLFW_VERSION}.tar.gz" "$tarball"
    extract_tarball "$tarball" "$source_dir"

    log "Building GLFW ${GLFW_VERSION}"
    cmake -S "$source_dir" -B "$build_subdir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DBUILD_SHARED_LIBS=OFF \
        -DGLFW_BUILD_DOCS=OFF \
        -DGLFW_BUILD_TESTS=OFF \
        -DGLFW_BUILD_EXAMPLES=OFF
    cmake --build "$build_subdir" --parallel "$CPU_COUNT"
    cmake --install "$build_subdir"

    touch "$build_marker"
    echo "$prefix"
}

main() {
    mkdir -p "$SRC_DIR" "$INSTALL_DIR"

    local openssl_prefix
    local glfw_prefix
    openssl_prefix="$(build_openssl)"
    glfw_prefix="$(build_glfw)"

    log "Configuring RSA_CPP"
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DOPENSSL_ROOT_DIR="$openssl_prefix" \
        -DCMAKE_PREFIX_PATH="$glfw_prefix"

    log "Building targets"
    cmake --build "$BUILD_DIR" --parallel "$CPU_COUNT"
}

main "$@"
