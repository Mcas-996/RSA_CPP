# RSA_CPP

Chinese version: [README_zh.md](README_zh.md)

An RSA encryption/decryption library implemented in C++, providing both Graphical User Interface (GUI) and Command Line Interface (CLI) versions.

## Project Overview

This project implements a complete RSA public-key encryption algorithm, including:
- Large prime number generation
- Key pair generation
- Text encryption/decryption
- Complete encryption workflow supporting ASCII characters

The project provides two interfaces:
- **GUI Version**: Modern GUI application based on ImGui (DirectX 11 on Windows, GLFW/OpenGL on Linux/macOS)
- **CLI Version**: Traditional command-line tool suitable for scripting and automation

> **Note**: Chinese documentation is available in [README_zh.md](README_zh.md)

## Features

- Generate RSA key pairs (demo uses 6-digit primes for clarity)
- Encrypt and decrypt ASCII/UTF-8 text from either GUI or CLI workflows
- Convert ciphertext to and from Base64, including binary file helpers
- Cross-platform GUI powered by ImGui (DirectX 11 on Windows, GLFW/OpenGL on Linux & macOS)
- CLI workflow designed for scripting, regression testing, and automation
- OpenSSL-backed cryptographic helpers with fast modular exponentiation

## Project Structure

```
RSA_CPP/
  CMakeLists.txt            # Top-level CMake build script
  main.cpp                  # GUI entry point (ImGui)
  main_cli.cpp              # CLI entry point
  RSA.hpp                   # Core RSA implementation
  bin.hpp                   # Binary/Base64 utilities
  prepare.cpp/.hpp          # Platform setup helpers
  Iwanna.hpp                # GUI logic
  ImGui/                    # ImGui source files
  third_party/cppcodec/     # Base64 codec dependency (cloned automatically in CI)
  build/                    # Generated build directory (ignored in VCS)
```

## Building and Running

### Common Prerequisites
- C++17 compatible compiler (GCC, Clang, or MSYS2 MinGW GCC)
- CMake 3.15 or newer
- OpenSSL development headers and libraries
- OpenGL 3.0+ and GLFW 3 for GUI builds
- Git (use `git clone --recursive` or run `git submodule update --init --recursive` to ensure `third_party/cppcodec` is present when building locally)

### Windows (MSYS2 / MinGW)
```
# Run inside the MSYS2 MinGW 64-bit shell
pacman -S --needed base-devel git mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja mingw-w64-x86_64-openssl mingw-w64-x86_64-glfw
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Linux (Debian/Ubuntu example)
```
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config libgl1-mesa-dev libglfw3-dev libssl-dev
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### macOS (Homebrew)
```
brew update
brew install cmake ninja glfw openssl@3
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
cmake --build build
```

### Build Outputs
- `build/RSA_CPP` (`RSA_CPP.exe` on Windows) - GUI application
- `build/RSA_CLI` (`RSA_CLI.exe` on Windows) - CLI utility
- macOS builds also produce `build/RSA_CPP.app` (binary inside `Contents/MacOS/`)

## Continuous Integration & Artifacts

GitHub Actions workflow `.github/workflows/cmake-multi-platform.yml` builds the project on Ubuntu (GCC & Clang), macOS (Clang), and Windows (MSYS2 MinGW). Each run installs the required dependencies, ensures `third_party/cppcodec` is present, configures CMake, compiles both targets, runs `ctest`, and uploads the resulting executables as artifacts named `RSA_CPP-<os>-<compiler>`. You can download these prebuilt binaries from the **Artifacts** panel of a successful workflow run.

## Usage Instructions

### GUI Version

After launching the GUI, you will see:
1. **Key Input Window**: Enter public key, private key, and modulus
2. **Key Generation Window**: Generate new RSA key pairs
3. **Key Display Window**: View current key information
4. **Encryption/Decryption Window**: Perform text encryption and decryption operations

### CLI Version

After launching the CLI version, you can use the following options:

```
RSA Encryption/Decryption CLI Tool
===================================

Options:
1. Generate new key pair
2. Input existing key pair
3. Show current key pair
4. Encrypt text
5. Decrypt text
6. Save string to binary file
7. Encrypt binary file
8. Decrypt Base64 ciphertext to binary string
9. Load binary file into string
10. Exit
```

Notes about workflow
- The program keeps an in-memory buffer `result` that stores the latest produced string. This can be either plaintext, Base64 ciphertext, or raw binary data represented as a string of bytes.
- Option 6 writes any current string as-is to a file in binary mode. It does not perform Base64 decoding automatically. Use option 8 to decrypt Base64 into raw bytes first, then use option 6 to persist them.
- Options 7/8/9 are convenient helpers for binary workflows:
  - 7 reads a binary file, encrypts it, and prints/keeps Base64 in memory.
  - 8 accepts Base64 (or comma-separated numeric ciphertext), decrypts it into raw bytes stored in memory, with a Base64 preview for verification.
  - 9 loads any binary file into memory (no encryption/decryption), also showing a Base64 preview.

#### Usage Examples

1. **Generate new key pair**
   ```
   Choose an option (1-6): 1
   Generating new key pair...
   RSA Key Information:
   Public Key (e): 65537
   Private Key (d): 12345678901234567890
   Modulus (n): 9876543210987654321
   ```

2. **Encrypt text**
   ```
   Choose an option (1-6): 4
   Enter text to encrypt: Hello, World!
   Encrypted text: 123456789,987654321,555666777,...
   ```

3. **Decrypt text**
   ```
   Choose an option (1-6): 5
   Enter ciphertext to decrypt (comma-separated): 123456789,987654321,555666777,...
   Decrypted text: Hello, World!
   ```

## API Documentation

### RSA Namespace

#### Core Structure
```cpp
struct KeyPair {
    std::string publicKey;    // Public key e
    std::string privateKey;   // Private key d
    std::string modulus;      // Modulus n
};
```

#### Main Functions

##### Key Generation
```cpp
KeyPair generateKeyPair();
// Generate RSA key pair, returns structure containing public key, private key, and modulus
```

##### Encryption/Decryption
```cpp
std::vector<long long> encryptText(const std::string& plaintext, const KeyPair& keyPair);
// Encrypt text, returns vector of encrypted numbers

std::string decryptText(const std::vector<long long>& ciphertext, const KeyPair& keyPair);
// Decrypt ciphertext, returns original text
```

##### Utility Functions
```cpp
std::string ciphertextToString(const std::vector<long long>& ciphertext);
// Convert ciphertext vector to string format

std::vector<long long> stringToCiphertext(const std::string& str);
// Convert string format back to ciphertext vector

void printKeyInfo(const KeyPair& keyPair);
// Print key information
```

## Technical Implementation

### RSA Algorithm Implementation

1. **Prime Generation**: Generates 6-digit primes using optimized primality testing
2. **Key Generation**: 
   - Selects two large prime numbers p and q
   - Calculates modulus n = p * q
   - Calculates Euler's totient function φ(n) = (p-1)(q-1)
   - Selects public exponent e (typically 65537)
   - Calculates private exponent d = e^(-1) mod φ(n)

3. **Encryption/Decryption**:
   - Encryption: c = m^e mod n
   - Decryption: m = c^d mod n

### Performance Optimizations
- **Fast Modular Exponentiation**: Uses square-and-multiply algorithm to optimize large number exponentiation
- **Extended Euclidean Algorithm**: Efficiently calculates modular inverse
- **Character-level Encryption**: Supports per-character encryption for ASCII characters

## Security Considerations

1. **Key Management**: Keep your private key secure and never share it with others
2. **Key Length**: Current implementation uses 6-digit primes (approximately 20-bit modulus), suitable for learning and demonstration purposes
3. **Production Use**: For production environments, use longer keys (2048-bit or higher) and specialized big integer libraries
4. **Text Limitations**: Character values being encrypted must be less than the modulus n
5. **Algorithm Limitations**: Due to using `long long` data type, key length is limited by 64-bit integer range

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contributions

Contributions are welcome! Please submit Issues and Pull Requests to improve this project.

## Author

Mcas-996

## Changelog

### v0.0.1
- Initial release
- Implemented complete RSA algorithm
- Provided both GUI and CLI interfaces
- Supported text encryption/decryption functionality
4. **Encrypt a binary file and get Base64**
   ```
   Choose an option (1-10): 7
   Enter source binary file path: ./data/input.bin
   Encrypted Base64 (use option 6 to save if needed):
   <Base64 ciphertext here>
   ```

5. **Decrypt Base64 ciphertext and save binary output**
   ```
   Choose an option (1-10): 8
   Enter Base64 ciphertext (or comma-separated numbers): <paste here>
   Decrypted binary data stored in memory (use option 6 to save).
   Base64 preview: <preview>

   Choose an option (1-10): 6
   Enter string to save (leave empty to use the last encryption result):
   <press Enter>
   Enter target file path: ./data/output.bin
   String saved to file: "./data/output.bin"
   ```

6. **Load a binary file into memory and re-save**
   ```
   Choose an option (1-10): 9
   Enter binary file path: ./data/input.bin
   Binary file loaded into memory (use option 6 to save elsewhere).
   Base64 preview: <preview>

   Choose an option (1-10): 6
   Enter string to save (leave empty to use the last encryption result):
   <press Enter>
   Enter target file path: ./data/copy.bin
   String saved to file: "./data/copy.bin"
   ```






