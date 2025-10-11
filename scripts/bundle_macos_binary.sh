#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 4 ]]; then
    echo "Usage: $0 <binary_path> <output_dir> <openssl_prefix> <glfw_prefix_or_dash>" >&2
    exit 1
fi

binary_path="$1"
output_dir="$2"
openssl_prefix="$3"
glfw_prefix="$4"

if [[ ! -f "$binary_path" ]]; then
    echo "Binary '$binary_path' not found" >&2
    exit 1
fi

mkdir -p "$output_dir"

binary_name="$(basename "$binary_path")"
bundled_binary="$output_dir/$binary_name"
cp "$binary_path" "$bundled_binary"
chmod +w "$bundled_binary"

declare -a library_paths

openssl_lib_dir="$openssl_prefix/lib"
if [[ ! -d "$openssl_lib_dir" ]]; then
    echo "OpenSSL lib dir '$openssl_lib_dir' not found" >&2
    exit 1
fi

for lib in libcrypto.3.dylib libssl.3.dylib; do
    full_path="$openssl_lib_dir/$lib"
    if [[ -f "$full_path" ]]; then
        library_paths+=("$full_path")
    fi
done

if [[ "$glfw_prefix" != "-" ]]; then
    glfw_lib_dir="$glfw_prefix/lib"
    glfw_lib="$glfw_lib_dir/libglfw.3.dylib"
    if [[ -f "$glfw_lib" ]]; then
        library_paths+=("$glfw_lib")
    else
        echo "GLFW library '$glfw_lib' not found" >&2
        exit 1
    fi
fi

declare -a bundled_libs
for src in "${library_paths[@]}"; do
    lib_name="$(basename "$src")"
    dest="$output_dir/$lib_name"
    cp "$src" "$dest"
    chmod +w "$dest"
    install_name_tool -id "@loader_path/$lib_name" "$dest"
    bundled_libs+=("$dest")
done

install_name_tool -add_rpath "@loader_path" "$bundled_binary" || true

for src in "${library_paths[@]}"; do
    lib_name="$(basename "$src")"
    install_name_tool -change "$src" "@loader_path/$lib_name" "$bundled_binary" || true
done

if [[ "$glfw_prefix" != "-" ]]; then
    install_name_tool -change "@rpath/libglfw.3.dylib" "@loader_path/libglfw.3.dylib" "$bundled_binary" || true
fi

for lib_path in "${bundled_libs[@]}"; do
    for src in "${library_paths[@]}"; do
        lib_name="$(basename "$src")"
        install_name_tool -change "$src" "@loader_path/$lib_name" "$lib_path" || true
    done
done

echo "Bundled '$binary_name' with libraries in '$output_dir'"
