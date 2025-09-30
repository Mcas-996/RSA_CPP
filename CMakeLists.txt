cmake_minimum_required(VERSION 3.15)

# 设置项目名称和支持的 C++ 标准
project(RSA_CPP LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加图形界面版本的可执行文件
add_executable(RSA_CPP
    main.cpp
    prepare.cpp
    prepare.hpp
    # 添加 ImGui 源文件
    ImGui/imgui.cpp
    ImGui/imgui_demo.cpp
    ImGui/imgui_draw.cpp
    ImGui/imgui_tables.cpp
    ImGui/imgui_widgets.cpp
    ImGui/imgui_impl_dx11.cpp
    ImGui/imgui_impl_win32.cpp
)

# 添加命令行界面版本的可执行文件
add_executable(RSA_CLI
    main_cli.cpp
    RSA.hpp
)

# 为图形界面版本配置包含目录和链接库
target_include_directories(RSA_CPP PRIVATE
    ${CMAKE_SOURCE_DIR}/ImGui
)

# 链接图形界面版本所需的系统库
if(WIN32)
    target_link_libraries(RSA_CPP PRIVATE
        d3d11       # DirectX 11
        dxgi        # DirectX Graphics Infrastructure
        d3dcompiler # D3D Compiler (needed for D3DCompile)
        user32      # Win32 API
        gdi32       # Graphics Device Interface
        ole32       # COM Library
        winmm       # Windows Multimedia
        xinput      # XInput API for game controllers
    )
endif()

# 为命令行版本配置包含目录
target_include_directories(RSA_CLI PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# 添加测试可执行文件
add_executable(test_rsa
    test_rsa.cpp
    RSA.hpp
)

target_include_directories(test_rsa PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# 添加调试可执行文件
add_executable(debug_primes
    debug_primes.cpp
    RSA.hpp
)

target_include_directories(debug_primes PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# 添加CLI自动化测试
add_executable(test_cli_auto
    test_cli_auto.cpp
    RSA.hpp
)

target_include_directories(test_cli_auto PRIVATE
    ${CMAKE_SOURCE_DIR}
)