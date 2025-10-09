# RSA_CPP

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

### Core Features
- ✅ RSA key pair generation (with large primes)
- ✅ Text encryption (supports ASCII characters)
- ✅ Text decryption
- ✅ Key import/export
- ✅ Ciphertext format conversion

### Technical Highlights
- 🔐 Uses large primes (6-digit range) for security
- ⚡ Fast modular exponentiation algorithm
- 🎯 Extended Euclidean algorithm for modular inverse calculation
- 📊 Comprehensive error handling and boundary checks

## Project Structure

```
RSA_CPP/RSA_CPP/
├── main.cpp              # GUI main program
├── main_cli.cpp          # CLI main program
├── RSA.hpp               # Core RSA algorithm implementation
├── prepare.hpp           # DirectX and ImGui initialization
├── Iwanna.hpp            # GUI interface logic
├── prepare.cpp           # DirectX device management
├── CMakeLists.txt        # CMake build configuration
├── ImGui/                # ImGui library files
└── build/                # Build output directory
```

## Building and Running

### Requirements
- **Operating System**: Windows, Linux, or macOS
- **Compiler**: C++17 compatible compiler (MSVC, GCC, Clang)
- **Build Tool**: CMake 3.15 or higher
- **GUI Dependencies**:
  - Windows: DirectX 11 (included with Windows SDK)
  - Linux/macOS: OpenGL 3.0+ and GLFW 3

For Linux/macOS you can install GLFW via your package manager:

```bash
# Debian/Ubuntu
sudo apt install libglfw3-dev

# macOS (Homebrew)
brew install glfw
```

### Build Steps

1. **Create build directory**
   ```bash
   mkdir build
   cd build
   ```

2. **Configure with CMake**
   ```bash
   cmake ..
   ```

3. **Build the project**
   ```bash
   cmake --build .
   ```

### Running the Program

After successful build, two executable files will be generated in the `build/` directory:

- **GUI Version**: `RSA_CPP` (`RSA_CPP.exe` on Windows)
  ```bash
  ./build/RSA_CPP
  ```

- **CLI Version**: `RSA_CLI` (`RSA_CLI.exe` on Windows)
  ```bash
  ./build/RSA_CLI
  ```

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
6. Exit
```

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