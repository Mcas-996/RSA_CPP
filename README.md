# RSA_CPP# RSA_CPP



An RSA encryption/decryption library implemented in C++, providing both Graphical User Interface (GUI) and Command Line Interface (CLI) versions.一个用C++实现的RSA加密解密库，提供图形界面(GUI)和命令行界面(CLI)两种使用方式。



## Project Overview## 项目简介



This project implements a complete RSA public-key encryption algorithm, including:本项目实现了完整的RSA公钥加密算法，包括：

- Large prime number generation- 大素数生成

- Key pair generation- 密钥对生成

- Text encryption/decryption- 文本加密/解密

- Complete encryption workflow supporting ASCII characters- 支持ASCII字符的完整加密流程



The project provides two interfaces:项目提供两种界面：

- **GUI Version**: Modern GUI application based on ImGui and DirectX 11- **图形界面版本**：基于ImGui和DirectX 11的现代化GUI应用

- **CLI Version**: Traditional command-line tool suitable for scripting and automation- **命令行版本**：传统的CLI工具，适合脚本和自动化使用



## Features## 功能特性



### Core Features### 核心功能

- ✅ RSA key pair generation (with large primes)- ✅ RSA密钥对生成（支持大素数）

- ✅ Text encryption (supports ASCII characters)- ✅ 文本加密（支持ASCII字符）

- ✅ Text decryption- ✅ 文本解密

- ✅ Key import/export- ✅ 密钥导入/导出

- ✅ Ciphertext format conversion- ✅ 密文格式转换



### Technical Highlights### 技术特点

- 🔐 Uses large primes (6-digit range) for security- 🔐 使用大素数（10^10到10^11范围）确保安全性

- ⚡ Fast modular exponentiation algorithm- ⚡ 快速模幂运算算法

- 🎯 Extended Euclidean algorithm for modular inverse calculation- 🎯 扩展欧几里得算法计算模逆

- 📊 Comprehensive error handling and boundary checks- 📊 完整的错误处理和边界检查



## Project Structure## 项目结构



``````

RSA_CPP/RSA_CPP/

├── main.cpp              # GUI main program├── main.cpp              # 图形界面主程序

├── main_cli.cpp          # CLI main program├── main_cli.cpp          # 命令行界面主程序

├── RSA.hpp               # Core RSA algorithm implementation├── RSA.hpp               # RSA算法核心实现

├── prepare.hpp           # DirectX and ImGui initialization├── prepare.hpp           # DirectX和ImGui初始化

├── Iwanna.hpp            # GUI interface logic├── Iwanna.hpp            # GUI界面逻辑

├── prepare.cpp           # DirectX device management├── prepare.cpp           # DirectX设备管理

├── CMakeLists.txt        # CMake build configuration├── CMakeLists.txt        # CMake构建配置

├── ImGui/                # ImGui library files├── ImGui/                # ImGui库文件

└── build/                # Build output directory└── build/                # 构建输出目录

``````



## Building and Running## 构建和运行



### Requirements### 环境要求

- **Operating System**: Windows- **操作系统**: Windows

- **Compiler**: C++17 compatible compiler (MSVC, GCC, Clang)- **编译器**: 支持C++17的编译器（MSVC, GCC, Clang）

- **Build Tool**: CMake 3.15 or higher- **构建工具**: CMake 3.15或更高版本

- **GUI Dependencies**: DirectX 11- **图形界面依赖**: DirectX 11



### Build Steps### 构建步骤



1. **Create build directory**1. **创建构建目录**

   ```bash   ```bash

   mkdir build   mkdir build

   cd build   cd build

   ```   ```



2. **Configure with CMake**2. **配置CMake**

   ```bash   ```bash

   cmake ..   cmake ..

   ```   ```



3. **Build the project**3. **编译项目**

   ```bash   ```bash

   cmake --build .   cmake --build .

   ```   ```



### Running the Program### 运行程序



After successful build, two executable files will be generated in the `build/` directory:构建成功后，会在`build/`目录下生成两个可执行文件：



- **GUI Version**: `RSA_CPP.exe`- **图形界面版本**: `RSA_CPP.exe`

  ```bash  ```bash

  ./build/RSA_CPP.exe  ./build/RSA_CPP.exe

  ```  ```



- **CLI Version**: `RSA_CLI.exe`- **命令行版本**: `RSA_CLI.exe`

  ```bash  ```bash

  ./build/RSA_CLI.exe  ./build/RSA_CLI.exe

  ```  ```



## Usage Instructions## 使用说明



### GUI Version### 图形界面版本



After launching the GUI, you will see:启动图形界面后，您将看到：

1. **Key Input Window**: Enter public key, private key, and modulus1. **密钥输入窗口**: 输入公钥、私钥和模数

2. **Key Generation Window**: Generate new RSA key pairs2. **密钥生成窗口**: 生成新的RSA密钥对

3. **Key Display Window**: View current key information3. **密钥显示窗口**: 查看当前密钥信息

4. **Encryption/Decryption Window**: Perform text encryption and decryption operations4. **加密/解密窗口**: 进行文本加密解密操作



### CLI Version### 命令行版本



After launching the CLI version, you can use the following options:启动CLI版本后，您可以使用以下功能：



``````

RSA Encryption/Decryption CLI ToolRSA Encryption/Decryption CLI Tool

======================================================================



Options:Options:

1. Generate new key pair1. Generate new key pair

2. Input existing key pair2. Input existing key pair

3. Show current key pair3. Show current key pair

4. Encrypt text4. Encrypt text

5. Decrypt text5. Decrypt text

6. Exit6. Exit

``````



#### Usage Examples#### 使用示例



1. **Generate new key pair**1. **生成新密钥对**

   ```   ```

   Choose an option (1-6): 1   Choose an option (1-6): 1

   Generating new key pair...   Generating new key pair...

   RSA Key Information:   RSA Key Information:

   Public Key (e): 65537   Public Key (e): 65537

   Private Key (d): 12345678901234567890   Private Key (d): 12345678901234567890

   Modulus (n): 9876543210987654321   Modulus (n): 9876543210987654321

   ```   ```



2. **Encrypt text**2. **加密文本**

   ```   ```

   Choose an option (1-6): 4   Choose an option (1-6): 4

   Enter text to encrypt: Hello, World!   Enter text to encrypt: Hello, World!

   Encrypted text: 123456789,987654321,555666777,...   Encrypted text: 123456789,987654321,555666777,...

   ```   ```



3. **Decrypt text**3. **解密文本**

   ```   ```

   Choose an option (1-6): 5   Choose an option (1-6): 5

   Enter ciphertext to decrypt (comma-separated): 123456789,987654321,555666777,...   Enter ciphertext to decrypt (comma-separated): 123456789,987654321,555666777,...

   Decrypted text: Hello, World!   Decrypted text: Hello, World!

   ```   ```



## API Documentation## API文档



### RSA Namespace### RSA命名空间



#### Core Structure#### 核心结构体

```cpp```cpp

struct KeyPair {struct KeyPair {

    std::string publicKey;    // Public key e    std::string publicKey;    // 公钥 e

    std::string privateKey;   // Private key d    std::string privateKey;   // 私钥 d

    std::string modulus;      // Modulus n    std::string modulus;      // 模数 n

};};

``````



#### Main Functions#### 主要函数



##### Key Generation##### 密钥生成

```cpp```cpp

KeyPair generateKeyPair();KeyPair generateKeyPair();

// Generate RSA key pair, returns structure containing public key, private key, and modulus// 生成RSA密钥对，返回包含公钥、私钥和模数的结构体

``````



##### Encryption/Decryption##### 加密解密

```cpp```cpp

std::vector<long long> encryptText(const std::string& plaintext, const KeyPair& keyPair);std::vector<long long> encryptText(const std::string& plaintext, const KeyPair& keyPair);

// Encrypt text, returns vector of encrypted numbers// 加密文本，返回加密后的数字数组



std::string decryptText(const std::vector<long long>& ciphertext, const KeyPair& keyPair);std::string decryptText(const std::vector<long long>& ciphertext, const KeyPair& keyPair);

// Decrypt ciphertext, returns original text// 解密密文，返回原始文本

``````



##### Utility Functions##### 工具函数

```cpp```cpp

std::string ciphertextToString(const std::vector<long long>& ciphertext);std::string ciphertextToString(const std::vector<long long>& ciphertext);

// Convert ciphertext vector to string format// 将密文数组转换为字符串格式



std::vector<long long> stringToCiphertext(const std::string& str);std::vector<long long> stringToCiphertext(const std::string& str);

// Convert string format back to ciphertext vector// 将字符串格式转换回密文数组



void printKeyInfo(const KeyPair& keyPair);void printKeyInfo(const KeyPair& keyPair);

// Print key information// 打印密钥信息

``````



## Technical Implementation## 技术实现



### RSA Algorithm Implementation### RSA算法实现



1. **Prime Generation**: Generates 6-digit primes using optimized primality testing1. **素数生成**: 使用Miller-Rabin素性测试在10^10到10^11范围内生成大素数

2. **Key Generation**: 2. **密钥生成**: 

   - Selects two large prime numbers p and q   - 选择两个大素数p和q

   - Calculates modulus n = p * q   - 计算模数n = p * q

   - Calculates Euler's totient function φ(n) = (p-1)(q-1)   - 计算欧拉函数φ(n) = (p-1)(q-1)

   - Selects public exponent e (typically 65537)   - 选择公钥指数e（通常为65537）

   - Calculates private exponent d = e^(-1) mod φ(n)   - 计算私钥指数d = e^(-1) mod φ(n)



3. **Encryption/Decryption**:3. **加密解密**:

   - Encryption: c = m^e mod n   - 加密：c = m^e mod n

   - Decryption: m = c^d mod n   - 解密：m = c^d mod n



### Performance Optimizations### 性能优化



- **Fast Modular Exponentiation**: Uses square-and-multiply algorithm to optimize large number exponentiation- **快速模幂运算**: 使用平方-乘算法优化大数幂运算

- **Extended Euclidean Algorithm**: Efficiently calculates modular inverse- **扩展欧几里得算法**: 高效计算模逆

- **Character-level Encryption**: Supports per-character encryption for ASCII characters- **字符级加密**: 支持ASCII字符的逐字符加密



## Security Considerations## 安全注意事项



1. **Key Management**: Keep your private key secure and never share it with others1. **密钥管理**: 请妥善保管私钥，不要泄露给他人

2. **Key Length**: Current implementation uses 6-digit primes (approximately 20-bit modulus), suitable for learning and demonstration purposes2. **密钥长度**: 当前实现使用6位素数（约20位模数），适合学习和演示用途

3. **Production Use**: For production environments, use longer keys (2048-bit or higher) and specialized big integer libraries3. **生产环境**: 如需用于生产环境，建议使用更长的密钥（2048位或更高）和专门的大数库

4. **Text Limitations**: Character values being encrypted must be less than the modulus n4. **文本限制**: 加密的文本字符值必须小于模数n

5. **Algorithm Limitations**: Due to using `long long` data type, key length is limited by 64-bit integer range5. **算法限制**: 由于使用`long long`数据类型，密钥长度受到64位整数范围限制



## License## 许可证



This project is licensed under the MIT License. See the LICENSE file for details.本项目采用MIT许可证，详见LICENSE文件。



## Contributions## 贡献



Contributions are welcome! Please submit Issues and Pull Requests to improve this project.欢迎提交Issue和Pull Request来改进这个项目！



## Author## 作者



Mcas-996Mcas-996



## Changelog## 更新日志



### v0.0.1### v0.0.1

- Initial release- 初始版本发布

- Implemented complete RSA algorithm- 实现完整的RSA算法

- Provided both GUI and CLI interfaces- 提供图形界面和命令行界面

- Supported text encryption/decryption functionality- 支持文本加密解密功能