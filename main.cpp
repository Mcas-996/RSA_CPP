#include "prepare.hpp"
#include "Iwanna.hpp"
#include <cstdlib>
int main() {
    //std::cout << "Application starting..." << std::endl;

    // 1. Create window (simplified)
    WNDCLASSEX wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, _T("ImGui Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // 2. Initialize D3D (assuming CreateDeviceD3D internally handles render target creation)
    if (!CreateDeviceD3D(hwnd)) return 1; // Simplified error handling

    ShowWindow(hwnd, SW_SHOWDEFAULT); UpdateWindow(hwnd);

    // 3. Initialize ImGui
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    // Add before main loop
    ImVec4 bg = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    //std::cout << "Initialization complete, entering main loop..." << std::endl;

    // 4. Main loop
    MSG msg = {};
    ZeroMemory(&msg, sizeof(msg));
    int frameCount = 0; // For debugging

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

        // Always execute rendering (this is key!)
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();
        Myspace::input_KP_window();
        Myspace::Generate_KP_window();
        Myspace::Show_KP_window();
        Myspace::enpt();
        Myspace::dept();
        Myspace::show_res();
        ImGui::Render();

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&bg);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // 5. Cleanup
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClass(wc.lpszClassName, wc.hInstance); // [[5]]
    return (int)msg.wParam;
}