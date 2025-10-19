# RSA_CPP 版本更新说明

## 本次亮点
- **跨平台构建稳定性**
  - 修复 MSYS2/Clang 运行器在链接对话框辅助库时出现的缺失符号问题。
  - 将通用的对话框工具函数集中到独立的源文件中，确保非 Windows 平台也能顺利编译。
- **macOS 构建体验优化**
  - 新增 `scripts/build_mac.sh`，可自动下载并编译 OpenSSL 3.3.1 与 GLFW 3.4，然后配置并构建整个项目。
  - 更新英文与中文 README，补充 macOS 自动脚本与手动（Homebrew）两种构建方式的说明。

## 构建与使用
- macOS 用户可先安装 Xcode Command Line Tools（运行 `xcode-select --install`），再执行 `scripts/build_mac.sh` 获取完整的自动化依赖与工程编译流程。
- Windows（MSYS2）与 Linux 的标准 CMake/Ninja 构建流程保持不变。

## 已知事项
- GitHub Actions 当前已生成 Windows 平台构建产物；macOS 和 Linux 的工作流需在更新后的代码基础上重新运行。
- 若需要手动重新编译依赖，可删除 `build/deps/install` 下对应的子目录后重新执行脚本。

感谢使用最新版本的 RSA_CPP！如遇问题，欢迎在仓库中提交 Issue 与我们交流。
