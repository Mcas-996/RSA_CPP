#!/usr/bin/env bash
set -euo pipefail

# Usage: run this script from the unpacked directory that contains RSA_CLI / RSA_CPP and .dylib files.

target_dir="$(pwd)"

echo "Signing helper libraries and executables under $target_dir ..."

find "$target_dir" -maxdepth 1 -type f \( -name "*.dylib" -o -perm -111 \) -print0 | while IFS= read -r -d '' file; do
    echo "Signing $file"
    codesign --force --sign - "$file"
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
    codesign --force --deep --sign - "$bin"
done

echo "Done. To verify, run:"
for bin in "${main_bin_candidates[@]}"; do
    echo "  codesign --verify --deep --strict --verbose=2 \"$bin\""
    echo "  spctl --assess --type execute --verbose \"$bin\""
done
