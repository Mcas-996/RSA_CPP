#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <binary_path> <output_dir>" >&2
    exit 1
fi

binary_path="$1"
output_dir="$2"

if [[ ! -f "$binary_path" ]]; then
    echo "Binary '$binary_path' not found" >&2
    exit 1
fi

binary_name="$(basename "$binary_path")"

mkdir -p "$output_dir"
bundled_binary="$output_dir/$binary_name"
cp "$binary_path" "$bundled_binary"
chmod +w "$bundled_binary"

declare -a skip_libs=(
    "linux-vdso.so.1"
    "libc.so.6"
    "libm.so.6"
    "libpthread.so.0"
    "libdl.so.2"
    "libstdc++.so.6"
    "libgcc_s.so.1"
    "ld-linux-x86-64.so.2"
)

should_skip() {
    local lib_name="$1"
    for skip in "${skip_libs[@]}"; do
        if [[ "$lib_name" == "$skip" ]]; then
            return 0
        fi
    done
    return 1
}

declare -a copied_libs=()
while IFS= read -r line; do
    path=""
    if [[ "$line" =~ "=>" ]]; then
        path="$(awk '{print $3}' <<<"$line")"
    else
        path="$(awk '{print $1}' <<<"$line")"
    fi

    if [[ -z "$path" || "$path" == "not" ]]; then
        continue
    fi

    if [[ ! -f "$path" ]]; then
        continue
    fi

    lib_name="$(basename "$path")"
    if should_skip "$lib_name"; then
        continue
    fi

    dest="$output_dir/$lib_name"
    if [[ ! -f "$dest" ]]; then
        cp "$path" "$dest"
        chmod +w "$dest"
        copied_libs+=("$dest")
    fi
done < <(ldd "$bundled_binary")

patchelf --set-rpath '$ORIGIN' "$bundled_binary"

for lib in "${copied_libs[@]}"; do
    patchelf --set-rpath '$ORIGIN' "$lib" || true
done

echo "Bundled '$binary_name' with libraries in '$output_dir'"
