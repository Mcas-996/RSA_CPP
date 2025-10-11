# RSA_CPP 项目说明（中文）

这是一个使用 C++ 编写的 RSA 加解密示例项目，同时提供图形界面 (GUI) 与命令行界面 (CLI)，适合学习、演示和验证 RSA 公钥体系的工作流程。

## 项目概述

项目包含以下核心能力：
- 生成示例 RSA 密钥对（演示版默认使用 6 位质数）
- 文本加解密，支持 ASCII/UTF-8 字符
- Base64 编码/解码与二进制数据处理
- GUI 与 CLI 双形态应用，方便在桌面环境或脚本环境中使用

## 功能亮点

- 快速生成并展示示例级 RSA 密钥对
- GUI 端提供一站式密钥管理、明文/密文切换与 Base64 预览
- CLI 端支持批处理脚本、回归测试等自动化场景
- 内置二进制文件与 Base64 的互转工具，简化文件加解密流程
- 跨平台图形界面基于 ImGui：Windows 使用 DirectX 11，Linux/macOS 使用 GLFW + OpenGL
- 使用 OpenSSL 进行大整数运算，结合快速模幂与扩展欧几里得算法

## 目录结构

```
RSA_CPP/
  CMakeLists.txt            # 顶层 CMake 构建脚本
  main.cpp                  # GUI 程序入口 (ImGui)
  main_cli.cpp              # CLI 程序入口
  RSA.hpp                   # RSA 算法实现
  bin.hpp                   # 二进制/Base64 工具函数
  prepare.cpp/.hpp          # 平台初始化与图形上下文封装
  Iwanna.hpp                # GUI 逻辑
  ImGui/                    # ImGui 源码
  third_party/cppcodec/     # Base64 编解码依赖（CI 中自动拉取）
  build/                    # 本地构建输出目录（默认忽略）
```

## 构建与运行

### 通用依赖
- C++17 兼容编译器（GCC、Clang 或 MSYS2 MinGW GCC）
- CMake 3.15 及以上版本
- OpenSSL 开发头文件与库
- OpenGL 3.0+ 与 GLFW 3（用于 GUI 版本）
- Git：请使用 `git clone --recursive` 或执行 `git submodule update --init --recursive` 以确保本地存在 `third_party/cppcodec`

### Windows（MSYS2 / MinGW）
```
# 在 MSYS2 MinGW 64-bit shell 中执行
pacman -S --needed base-devel git mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja mingw-w64-x86_64-openssl mingw-w64-x86_64-glfw
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Linux（以 Debian/Ubuntu 为例）
```
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config libgl1-mesa-dev libglfw3-dev libssl-dev
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### macOS（Homebrew）
```
brew update
brew install cmake ninja glfw openssl@3
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
cmake --build build
```

### 构建产物
- `build/RSA_CPP`（Windows 下为 `RSA_CPP.exe`）：GUI 程序
- `build/RSA_CLI`（Windows 下为 `RSA_CLI.exe`）：命令行工具
- macOS 额外生成 `build/RSA_CPP.app`，可执行文件位于 `Contents/MacOS/`

## 持续集成与构建产物

仓库内的 `.github/workflows/cmake-multi-platform.yml` 工作流会在以下平台自动构建：
- Ubuntu：GCC 与 Clang 两套工具链
- macOS：Clang + Homebrew 依赖
- Windows：MSYS2 MinGW 环境

工作流会自动安装依赖、确保 `third_party/cppcodec` 可用、执行 CMake 配置/编译与 `ctest`，并将生成的可执行文件打包为 `RSA_CPP-<os>-<compiler>` 形式的构建产物。可在任意一次成功运行的 GitHub Actions 记录中，通过页面右上角的 **Artifacts** 面板下载对应平台的二进制文件。

## CLI 使用说明

启动 CLI 后会看到如下菜单：
```
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

使用要点：
- 程序维护一个内存缓存 `result`，用于保存最近一次操作的字符串。它可能是明文、Base64 密文或原始二进制数据（以字节串形式保存）。
- 选项 6 会直接将当前字符串按原样写入文件，并不会自动 Base64 解码；若要恢复二进制数据，请先使用选项 8。
- 选项 7/8/9 可用于常见的文件工作流：
  - 7：读取二进制文件、加密并输出 Base64，同时将结果写入缓存
  - 8：输入 Base64（或逗号分隔的数字密文），解密得到原始字节并存入缓存，并提供 Base64 预览
  - 9：直接将任意二进制文件载入缓存，便于后续保存或加密

### 操作示例

1) 加密任意二进制文件并得到 Base64
```
Choose an option (1-10): 7
Enter source binary file path: ./data/input.bin
Encrypted Base64 (use option 6 to save if needed):
<Base64 ciphertext here>
```

2) 解密 Base64 密文并保存为文件
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

3) 读取二进制文件到内存并重新保存
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

## 开发与安全说明

- 演示实现使用 `long long` 存储大整数，密钥规模约为 6 位质数 * 6 位质数，适用于教学演示，不适合生产环境。
- 加密文本的字符值需小于模数 `n`，请根据密钥规模控制输入。
- 如需生产级安全性，请改用 2048 位及以上大整数库，并配合安全随机数生成器。

## 许可证

- 本项目采用 MIT 许可证，详情参见仓库中的 `LICENSE` 文件。

## 贡献

欢迎通过 Issue 或 Pull Request 提交问题与改进建议。

## 作者

- 作者：Mcas-996

## 更新日志

- v0.0.1：初始版本，提供 GUI 与 CLI，覆盖完整的文本与文件加解密流程。
