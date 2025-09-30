#ifndef PREPARE_HPP // Prevent duplicate header inclusion
#define PREPARE_HPP

#include <d3d11.h> // Make sure to include necessary headers
#include <windows.h> // Required for HWND
#include "Auto.h"
#include "ImGui/imconfig.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imstb_rectpack.h"
#include "ImGui/imstb_textedit.h"
#include "ImGui/imstb_truetype.h"

#include <tchar.h>
#pragma comment(lib, "d3d11.lib")
// Declare global variables (using extern)
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
// extern HWND hwnd; // hwnd is defined/used in main and WndProc functions

// Function declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif // PREPARE_HPP