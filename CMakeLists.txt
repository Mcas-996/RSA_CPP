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
    $<$<PLATFORM_ID:Windows>:ImGui/imgui_impl_dx11.cpp>
    $<$<PLATFORM_ID:Windows>:ImGui/imgui_impl_win32.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:ImGui/imgui_impl_glfw.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:ImGui/imgui_impl_opengl3.cpp>
)

# 添加命令行界面版本的可执行文件
add_executable(RSA_CLI
    main_cli.cpp
    RSA.hpp
)

# 查找依赖包
find_package(OpenGL REQUIRED)

# Try to find GLFW
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(glfw3 glfw3)
    endif()
endif()

if(NOT glfw3_FOUND)
    message(WARNING "GLFW not found. GUI will not be available on Linux/macOS.")
    target_compile_definitions(RSA_CPP PRIVATE NO_GLFW)
else()
    target_compile_definitions(RSA_CPP PRIVATE HAS_GLFW)
endif()

# 为图形界面版本配置包含目录和链接库
target_include_directories(RSA_CPP PRIVATE
    ${CMAKE_SOURCE_DIR}/ImGui
    ${OPENGL_INCLUDE_DIRS}
    ${glfw3_INCLUDE_DIRS}
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
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)

    target_link_libraries(RSA_CPP PRIVATE
        OpenGL::GL
        glfw
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(RSA_CPP PRIVATE GL_SILENCE_DEPRECATION)
else()
    target_link_libraries(RSA_CPP PRIVATE
        OpenGL::GL
        glfw
        ${CMAKE_DL_LIBS}
    )
endif()

# 为命令行版本配置包含目录
target_include_directories(RSA_CLI PRIVATE
    ${CMAKE_SOURCE_DIR}
)
