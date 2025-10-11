cmake_minimum_required(VERSION 3.15)

project(RSA_CPP LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RSA_CPP
    main.cpp
    prepare.cpp
    prepare.hpp
    ImGui/imgui.cpp
    ImGui/imgui_demo.cpp
    ImGui/imgui_draw.cpp
    ImGui/imgui_tables.cpp
    ImGui/imgui_widgets.cpp
    ImGui/misc/cpp/imgui_stdlib.cpp
    $<$<PLATFORM_ID:Windows>:ImGui/imgui_impl_dx11.cpp>
    $<$<PLATFORM_ID:Windows>:ImGui/imgui_impl_win32.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:ImGui/imgui_impl_glfw.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:ImGui/imgui_impl_opengl3.cpp>
)

add_executable(RSA_CLI
    main_cli.cpp
    RSA.hpp
)

find_package(OpenGL REQUIRED)
find_package(OpenSSL REQUIRED)

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

set(GLFW_LINK_TARGET "")
if(glfw3_FOUND)
    if(TARGET glfw)
        set(GLFW_LINK_TARGET glfw)
    elseif(TARGET glfw::glfw)
        set(GLFW_LINK_TARGET glfw::glfw)
    elseif(glfw3_LIBRARIES)
        set(GLFW_LINK_TARGET ${glfw3_LIBRARIES})
    else()
        set(GLFW_LINK_TARGET glfw)
    endif()
endif()

target_include_directories(RSA_CPP PRIVATE
    ${CMAKE_SOURCE_DIR}/ImGui
    ${OPENGL_INCLUDE_DIRS}
    ${glfw3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

if(WIN32)
    target_link_libraries(RSA_CPP PRIVATE
        d3d11
        dxgi
        d3dcompiler
        user32
        gdi32
        ole32
        winmm
        xinput
    )
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)

    target_link_libraries(RSA_CPP PRIVATE
        OpenGL::GL
        ${GLFW_LINK_TARGET}
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${CMAKE_DL_LIBS}
    )

    target_compile_definitions(RSA_CPP PRIVATE GL_SILENCE_DEPRECATION)
else()
    target_link_libraries(RSA_CPP PRIVATE
        OpenGL::GL
        ${GLFW_LINK_TARGET}
        ${CMAKE_DL_LIBS}
    )
endif()

target_link_libraries(RSA_CPP PRIVATE OpenSSL::Crypto)

target_include_directories(RSA_CLI PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(RSA_CLI PRIVATE OpenSSL::Crypto)
