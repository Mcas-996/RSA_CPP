#!/usr/bin/env bash
set -euo pipefail

# Usage: ./macOS_sign.sh <directory-with-RSA-binaries>
# Example: ./macOS_sign.sh RSA_CLI_macOS

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <directory-with-RSA-binaries>" >&2
    exit 1
fi

target_dir="$1"

if [[ ! -d "$target_dir" ]]; then
    echo "Directory not found: $target_dir" >&2
    exit 1
fi

echo "Signing helper libraries and executables under $target_dir ..."

find "$target_dir" -type f \( -name "*.dylib" -o -perm -111 \) -print0 | while IFS= read -r -d '' file; do
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
