# RSA_CPP Release Notes
**version**: `
## Highlights
- **Cross-platform build stability**  
  - Fixed missing symbol issues when linking the dialog helper library on MSYS2/Clang runners.  
  - Ensured non-Windows targets compile cleanly by consolidating shared dialog utilities.
- **macOS improvements**  
  - Added `scripts/build_mac.sh`, an end-to-end build script that downloads and compiles OpenSSL 3.3.1 and GLFW 3.4 before configuring the project.  
  - Updated both English and Chinese README files with instructions for the automated macOS build path alongside the manual Homebrew option.

## Build & Usage
- Run `scripts/build_mac.sh` on macOS (after installing Xcode Command Line Tools) for a fully automated dependency and project build.
- Standard CMake/Ninja flows for Windows (MSYS2) and Linux remain unchanged.

## Known Notes
- GitHub Actions artifacts are produced for Windows; macOS and Linux workflows require the updated sources before re-running.
- When rebuilding dependencies manually, delete the relevant subdirectory under `build/deps/install` to force a clean build.

Enjoy hacking on RSA_CPP, and thanks for trying out the latest changes! If you encounter issues, please open an issue on the repository.
