#ifndef PREPARE_HPP // Prevent duplicate header inclusion
#define PREPARE_HPP

#include "Auto.h"
#include "ImGui/imconfig.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imstb_rectpack.h"
#include "ImGui/imstb_textedit.h"
#include "ImGui/imstb_truetype.h"

#if defined(_WIN32)
#include <d3d11.h> // Make sure to include necessary headers
#include <dxgi.h>
#include <windows.h> // Required for HWND
#include <tchar.h>
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#pragma comment(lib, "d3d11.lib")
// Windows specific global variables
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern HWND g_hwnd; // Main window handle

// Windows specific function declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#elif defined(HAS_GLFW)
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#endif
#include <GLFW/glfw3.h>
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
// Linux/macOS specific global variables
extern GLFWwindow* g_window;
#endif

#if defined(HAS_GLFW)
const char* GetOpenGLGLSLVersion();
#endif

// Cross-platform function declarations
bool InitializeWindowAndGraphics(int width, int height, const char* title);
void CleanupGraphicsAndWindow();
void NewFrame();
void Render();

#endif // PREPARE_HPP
