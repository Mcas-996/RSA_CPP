#include "prepare.hpp"
#include "Iwanna.hpp"
#include <cstdlib>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
extern LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

int main() {
    //std::cout << "Application starting..." << std::endl;

#if !defined(_WIN32) && defined(NO_GLFW)
    std::cerr << "Error: GLFW library not found. Cannot run GUI on Linux/macOS without GLFW." << std::endl;
    std::cerr << "Please install GLFW3 development libraries. On Debian/Ubuntu: sudo apt install libglfw3-dev" << std::endl;
    return 1;
#endif

    // 1. Create window and initialize graphics
    if (!InitializeWindowAndGraphics(1280, 800, "RSA_CPP GUI")) return 1;

    // 2. Initialize ImGui
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::StyleColorsDark();

#if defined(_WIN32)
    extern HWND g_hwnd;
    extern ID3D11Device* g_pd3dDevice;
    extern ID3D11DeviceContext* g_pd3dDeviceContext;
    ImGui_ImplWin32_Init(g_hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
#else
#if defined(HAS_GLFW)
    extern GLFWwindow* g_window;
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(GetOpenGLGLSLVersion());
#endif
#endif

    //std::cout << "Initialization complete, entering main loop..." << std::endl;

    // 4. Main loop
    int frameCount = 0; // For debugging

#if defined(_WIN32)
    MSG msg = {};
    ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT) {
        frameCount++;
        //if (frameCount % 60 == 0) { // Output every 60 frames
        //    std::cout << "Ran " << frameCount << " frames" << std::endl;
        //}

        // Process all pending messages
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
#else
    while (true) {  // For Linux, we handle loop exit differently
        frameCount++;
        //if (frameCount % 60 == 0) { // Output every 60 frames
        //    std::cout << "Ran " << frameCount << " frames" << std::endl;
        //}

#if defined(HAS_GLFW)
        extern GLFWwindow* g_window;
        if (glfwWindowShouldClose(g_window))
            break;

        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();
#endif
#endif

        // Always execute rendering (this is key!)
        NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();
        Myspace::input_KP_window();
        Myspace::Generate_KP_window();
        Myspace::Show_KP_window();
        Myspace::enpt();
        Myspace::dept();
        Myspace::show_res();

        Render();
    }

    // 5. Cleanup
#if defined(_WIN32)
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown();
#else
#if defined(HAS_GLFW)
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
#endif
#endif
    ImGui::DestroyContext();
    CleanupGraphicsAndWindow();
#if defined(_WIN32)
    return (int)msg.wParam;
#else
    return 0;
#endif
}
