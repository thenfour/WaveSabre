#include <d3d9.h>
#include <tchar.h>
#include <windows.h>

#include <imgui-knobs.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include <WaveSabreCore.h>
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>

#include "manualTestsUI.hpp"

#pragma comment(lib, "d3d9.lib")

void InitWaveSabre()
{
  WaveSabreCore::M7::math::InitLUTs();
}

static LPDIRECT3D9 g_pD3D = NULL;
static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS g_d3dpp = {};

// Defer window resize handling to a safe point (before starting a new frame)
static bool g_ResizePending = false;
static UINT g_ResizeWidth = 0;
static UINT g_ResizeHeight = 0;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool CreateDeviceD3D(HWND hWnd)
{
  if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
    return false;

  ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
  g_d3dpp.Windowed = TRUE;
  g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
  g_d3dpp.EnableAutoDepthStencil = TRUE;
  g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
  g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  if (g_pD3D->CreateDevice(
          D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
    return false;
  return true;
}

static void CleanupDeviceD3D()
{
  if (g_pd3dDevice)
  {
    g_pd3dDevice->Release();
    g_pd3dDevice = NULL;
  }
  if (g_pD3D)
  {
    g_pD3D->Release();
    g_pD3D = NULL;
  }
}

static void ResetDevice()
{
  ImGui_ImplDX9_InvalidateDeviceObjects();
  HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
  if (hr == D3DERR_INVALIDCALL)
    IM_ASSERT(0);
  ImGui_ImplDX9_CreateDeviceObjects();
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
  WNDCLASSEX wc = {sizeof(WNDCLASSEX),
                   CS_CLASSDC,
                   WndProc,
                   0L,
                   0L,
                   GetModuleHandle(NULL),
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   _T("WaveSabreManualTests"),
                   NULL};
  ::RegisterClassEx(&wc);
  HWND hwnd = ::CreateWindow(wc.lpszClassName,
                             _T("WaveSabre Manual Tests"),
                             WS_OVERLAPPEDWINDOW,
                             100,
                             100,
                             1280,
                             1200,
                             NULL,
                             NULL,
                             wc.hInstance,
                             NULL);

  if (!CreateDeviceD3D(hwnd))
  {
    CleanupDeviceD3D();
    return 1;
  }

  ::ShowWindow(hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(hwnd);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  // Disable imgui.ini load/save
  io.IniFilename = nullptr;

  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX9_Init(g_pd3dDevice);

  InitWaveSabre();

  WaveSabreVstLib::Maj7ManualTestState state;

  // Main loop
  bool done = false;
  while (!done)
  {
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
        done = true;
    }
    if (done)
      break;

    // Apply pending resize before beginning a new ImGui frame
    if (g_ResizePending && g_pd3dDevice != NULL)
    {
      if (g_ResizeWidth > 0 && g_ResizeHeight > 0)
      {
        g_d3dpp.BackBufferWidth = g_ResizeWidth;
        g_d3dpp.BackBufferHeight = g_ResizeHeight;
        ResetDevice();
      }
      g_ResizePending = false;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
      auto psynth = std::make_unique<WaveSabreVstLib::Maj7SynthWrapper>();
      WaveSabreVstLib::renderManualTestsUI(*psynth.get(), state);
    }

    ImGui::EndFrame();
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(0, 0, 0, 255);
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
    if (g_pd3dDevice->BeginScene() >= 0)
    {
      ImGui::Render();
      ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
      g_pd3dDevice->EndScene();
    }
    HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
      ResetDevice();
  }

  ImGui_ImplDX9_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();
  ::DestroyWindow(hwnd);
  ::UnregisterClass(wc.lpszClassName, wc.hInstance);

  return 0;
}

// Win32 message handler
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;
  switch (msg)
  {
    case WM_SIZE:
      // Defer device reset to the main loop to avoid interfering with ImGui frame lifecycle
      if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
      {
        g_ResizeWidth = LOWORD(lParam);
        g_ResizeHeight = HIWORD(lParam);
        g_ResizePending = true;
      }
      return 0;
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU)  // Disable ALT application menu
        return 0;
      break;
    case WM_DESTROY:
      ::PostQuitMessage(0);
      return 0;
  }
  return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
