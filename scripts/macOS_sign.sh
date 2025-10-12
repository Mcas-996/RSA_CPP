#!/usr/bin/env bash
set -euo pipefail

# Usage: run this script from the unpacked directory that contains RSA_CLI / RSA_CPP and the .dylib files.

target_dir="$(pwd)"

echo "Target directory: $target_dir"

if command -v xattr >/dev/null 2>&1; then
    echo "Clearing quarantine attributes (if any)..."
    xattr -dr com.apple.quarantine "$target_dir" 2>/dev/null || true
fi

echo "Signing helper libraries and executables under $target_dir ..."

find "$target_dir" -type f \( -name "*.dylib" -o -name "*.lib" -o -name "*.a" -o -perm -u+x -o -perm -g+x -o -perm -o+x \) -print0 |
while IFS= read -r -d '' file; do
    echo "Signing $file"
    codesign --force --sign - --timestamp=none "$file"
    codesign --verify --verbose=2 "$file"
done

main_bin_candidates=()
if [[ -f "$target_dir/RSA_CLI" ]]; then
    main_bin_candidates+=("$target_dir/RSA_CLI")
fi
if [[ -f "$target_dir/RSA_CPP" ]]; then
    main_bin_candidates+=("$target_dir/RSA_CPP")
fi

for bin in "${main_bin_candidates[@]}"; do
    echo "Applying deep sign to $bin"
    codesign --force --deep --sign - --timestamp=none "$bin"
    codesign --verify --deep --strict --verbose=2 "$bin"
    if command -v spctl >/dev/null 2>&1; then
        spctl --add --label "RSA_CPP_Local" "$bin" 2>/dev/null || true
    fi
done

echo "Done. To verify, run:"
for bin in "${main_bin_candidates[@]}"; do
    echo "  codesign --verify --deep --strict --verbose=2 \"$bin\""
    echo "  spctl --assess --type execute --verbose \"$bin\""
done
