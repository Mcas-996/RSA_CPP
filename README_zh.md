# RSA_CPP

一个用C++实现的RSA加密解密库，提供图形界面(GUI)和命令行界面(CLI)两种使用方式。

## 项目简介

本项目实现了完整的RSA公钥加密算法，包括：
- 大素数生成
- 密钥对生成
- 文本加密/解密
- 支持ASCII字符的完整加密流程

项目提供两种界面：
- **图形界面版本**：基于ImGui的现代化GUI应用（Windows使用DirectX 11，Linux/macOS使用GLFW+OpenGL）
- **命令行版本**：传统的CLI工具，适合脚本和自动化使用

## 功能特性

### 核心功能
- ✅ RSA密钥对生成（支持大素数）
- ✅ 文本加密（支持ASCII字符）
- ✅ 文本解密
- ✅ 密钥导入/导出
- ✅ 密文格式转换

### 技术特点
- 🔐 使用大素数（10^10到10^11范围）确保安全性
- ⚡ 快速模幂运算算法
- 🎯 扩展欧几里得算法计算模逆
- 📊 完整的错误处理和边界检查

## 项目结构

```
RSA_CPP/
├── main.cpp              # 图形界面主程序
├── main_cli.cpp          # 命令行界面主程序
├── RSA.hpp               # RSA算法核心实现
├── prepare.hpp           # DirectX和ImGui初始化
├── Iwanna.hpp            # GUI界面逻辑
├── prepare.cpp           # DirectX设备管理
├── CMakeLists.txt        # CMake构建配置
├── ImGui/                # ImGui库文件
└── build/                # 构建输出目录
```

## 构建和运行

### 环境要求
- **操作系统**: Windows、Linux或macOS
- **编译器**: 支持C++17的编译器（MSVC, GCC, Clang）
- **构建工具**: CMake 3.15或更高版本
- **图形界面依赖**:
  - Windows：DirectX 11（随Windows SDK提供）
  - Linux/macOS：OpenGL 3.0+ 和 GLFW 3

在Linux/macOS上可以通过包管理器安装GLFW：

```bash
# Debian/Ubuntu
sudo apt install libglfw3-dev

# macOS (Homebrew)
brew install glfw
```

### 构建步骤

1. **创建构建目录**
   ```bash
   mkdir build
   cd build
   ```

2. **配置CMake**
   ```bash
   cmake ..
   ```

3. **编译项目**
   ```bash
   cmake --build .
   ```

### 运行程序

构建成功后，会在`build/`目录下生成两个可执行文件：

- **图形界面版本**: `RSA_CPP`（Windows上为 `RSA_CPP.exe`）
  ```bash
  ./build/RSA_CPP
  ```

- **命令行版本**: `RSA_CLI`（Windows上为 `RSA_CLI.exe`）
  ```bash
  ./build/RSA_CLI
  ```

## 使用说明

### 图形界面版本

启动图形界面后，您将看到：
1. **密钥输入窗口**: 输入公钥、私钥和模数
2. **密钥生成窗口**: 生成新的RSA密钥对
3. **密钥显示窗口**: 查看当前密钥信息
4. **加密/解密窗口**: 进行文本加密解密操作

### 命令行版本

启动CLI版本后，您可以使用以下功能：

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

#### 使用示例

1. **生成新密钥对**
   ```
   Choose an option (1-6): 1
   Generating new key pair...
   RSA Key Information:
   Public Key (e): 65537
   Private Key (d): 12345678901234567890
   Modulus (n): 9876543210987654321
   ```

2. **加密文本**
   ```
   Choose an option (1-6): 4
   Enter text to encrypt: Hello, World!
   Encrypted text: 123456789,987654321,555666777,...
   ```

3. **解密文本**
   ```
   Choose an option (1-6): 5
   Enter ciphertext to decrypt (comma-separated): 123456789,987654321,555666777,...
   Decrypted text: Hello, World!
   ```

## API文档

### RSA命名空间

#### 核心结构体
```cpp
struct KeyPair {
    std::string publicKey;    // 公钥 e
    std::string privateKey;   // 私钥 d
    std::string modulus;      // 模数 n
};
```

#### 主要函数

##### 密钥生成
```cpp
KeyPair generateKeyPair();
// 生成RSA密钥对，返回包含公钥、私钥和模数的结构体
```

##### 加密解密
```cpp
std::vector<long long> encryptText(const std::string& plaintext, const KeyPair& keyPair);
// 加密文本，返回加密后的数字数组

std::string decryptText(const std::vector<long long>& ciphertext, const KeyPair& keyPair);
// 解密密文，返回原始文本
```

##### 工具函数
```cpp
std::string ciphertextToString(const std::vector<long long>& ciphertext);
// 将密文数组转换为字符串格式

std::vector<long long> stringToCiphertext(const std::string& str);
// 将字符串格式转换回密文数组

void printKeyInfo(const KeyPair& keyPair);
// 打印密钥信息
```

## 技术实现

### RSA算法实现

1. **素数生成**: 使用Miller-Rabin素性测试在10^10到10^11范围内生成大素数
2. **密钥生成**: 
   - 选择两个大素数p和q
   - 计算模数n = p * q
   - 计算欧拉函数φ(n) = (p-1)(q-1)
   - 选择公钥指数e（通常为65537）
   - 计算私钥指数d = e^(-1) mod φ(n)

3. **加密解密**:
   - 加密：c = m^e mod n
   - 解密：m = c^d mod n

### 性能优化

- **快速模幂运算**: 使用平方-乘算法优化大数幂运算
- **扩展欧几里得算法**: 高效计算模逆
- **字符级加密**: 支持ASCII字符的逐字符加密

## 安全注意事项

1. **密钥管理**: 请妥善保管私钥，不要泄露给他人
2. **密钥长度**: 当前实现使用6位素数（约20位模数），适合学习和演示用途
3. **生产环境**: 如需用于生产环境，建议使用更长的密钥（2048位或更高）和专门的大数库
4. **文本限制**: 加密的文本字符值必须小于模数n
5. **算法限制**: 由于使用`long long`数据类型，密钥长度受到64位整数范围限制

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目！

## 作者

Mcas-996

## 更新日志

### v0.0.1
- 初始版本发布
- 实现完整的RSA算法
- 提供图形界面和命令行界面
- 支持文本加密解密功能