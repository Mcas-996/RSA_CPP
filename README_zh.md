# RSA_CPP（中文说明）

一个用 C++ 实现的 RSA 加解密示例项目，同时提供图形界面（GUI）与命令行（CLI）。

本仓库适合学习与演示 RSA 基本流程，不建议直接用于生产环境安全需求。

## 项目概述

包含以下功能：
- 随机生成 6 位素数并构造 RSA 密钥对（e、d、n）
- 文本加解密（逐字符，适合 ASCII/UTF-8 场景）
- 密文格式转换（逗号分隔数字、Base64）
- 二进制文件加解密（通过字节串与 Base64 辅助）

提供两个入口：
- GUI：基于 ImGui（Windows 使用 DirectX 11；Linux/macOS 使用 GLFW + OpenGL）
- CLI：命令行工具，便于脚本化和快速验证

## 主要文件结构

```
RSA_CPP/
  ├─ main.cpp          # GUI 入口
  ├─ main_cli.cpp      # CLI 入口
  ├─ RSA.hpp           # RSA 核心（教学版，long long）
  ├─ bin.hpp           # 二进制文件 <-> std::string 工具
  ├─ prepare.hpp/.cpp  # GUI 平台初始化与设备管理
  ├─ Iwanna.hpp        # GUI 逻辑
  ├─ ImGui/            # ImGui 源码
  ├─ CMakeLists.txt    # 构建脚本
  └─ build/            # 构建输出目录
```

## 构建与运行

依赖：C++17 编译器、CMake >= 3.15；GUI 还需要（Win: DirectX 11 / Linux & macOS: OpenGL3 + GLFW3）。

通用构建步骤：
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

可执行文件（默认在 build/ 下）：
- GUI：`RSA_CPP`（Windows 为 `RSA_CPP.exe`）
- CLI：`RSA_CLI`（Windows 为 `RSA_CLI.exe`）

## CLI 使用说明

启动后将看到菜单：
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

工作方式与要点：
- 程序维护一个内存变量 `result`，用来暂存最近的“字符串结果”。
  - 可能是：明文、Base64 密文、或“原始二进制字节组成的字符串”。
- 第 6 项“保存字符串到二进制文件”是“原样写入”，不会自动做 Base64 解码。
  - 若你的数据是 Base64 文本，直接用 6 会把“Base64 字符”写成文件，而不是还原的二进制。
  - 还原场景：用 8 先解密 Base64 得到原始字节串 → 再用 6 写入文件。
- 二进制相关快捷项：
  - 7：读取二进制文件 → 加密 → 输出 Base64（存入 `result`）。
  - 8：输入 Base64/数字密文 → 解密 → 得到原始二进制（存入 `result`），并打印 Base64 预览。
  - 9：把任意二进制文件直接加载到 `result`（不加解密），也会给出 Base64 预览便于确认。

### 操作示例

1) 加密二进制文件并获得 Base64
```
Choose an option (1-10): 7
Enter source binary file path: ./data/input.bin
Encrypted Base64 (use option 6 to save if needed):
<Base64 ciphertext here>
```

2) 将 Base64 密文解密为原始二进制并保存为文件
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

3) 读取一个二进制文件到内存并另存
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

## 技术与限制（教学版说明）

- 本实现使用 `long long`，素数范围为 6 位数量级，仅用于演示流程与原理。
- 加解密为“逐字符”模式，字符值需要小于模数 `n`。
- 如需生产可用的强安全性，请使用大整数库与 2048 位及以上密钥长度。

## 许可与贡献

- 许可：MIT（详见 `LICENSE`）。
- 欢迎通过 Issue 或 PR 贡献代码与建议。

## 作者与更新

- 作者：Mcas-996
- 更新日志：
  - v0.0.1 初始版本，提供 GUI 与 CLI，支持文本与二进制辅助流程。

