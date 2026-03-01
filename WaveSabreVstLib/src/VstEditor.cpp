#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <WaveSabreVstLib/VstEditor.h>

#include <sstream>

using namespace std;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void* hInstance;

static std::atomic_bool gIsRendering = false;

namespace WaveSabreVstLib
{
VstEditor::VstEditor(AudioEffect* audioEffect, int width, int height)
    : AEffEditor(audioEffect)
{
  audioEffect->setEditor(this);
  auto r = CoInitialize(0);

  mDefaultRect.right = width;
  mDefaultRect.bottom = height;
  setKnobMode(2 /*kLinearMode*/);
}

VstEditor::~VstEditor()
{
  CoUninitialize();
}

void VstEditor::Window_Open(HWND parentWindow)
{
  //cc::LogScope ls("Window_Open");
  static constexpr wchar_t const* const className = L"wavesabre vst";
  WNDCLASSEXW wc = {
      sizeof(wc), CS_DBLCLKS, gWndProc, 0L, 0L, (HINSTANCE)::hInstance, NULL, NULL, NULL, NULL, className, NULL};
  ::RegisterClassExW(&wc);

  HWND hwnd = CreateWindowExW(0,
                              className,
                              nullptr,
                              WS_CHILD,
                              0,
                              0,
                              (mDefaultRect.right - mDefaultRect.left),
                              (mDefaultRect.bottom - mDefaultRect.top),
                              parentWindow,
                              (HMENU)1,
                              (HINSTANCE)::hInstance,
                              this);

  mParentWindow = parentWindow;

  SetTimer(hwnd, 0, 16, 0);

  mCurrentWindow = hwnd;
  SetPropW(hwnd, L"WSGuiInstance", (HANDLE)this);
  SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)gWndProc);

  // Initialize Direct3D
  if (!CreateDeviceD3D(hwnd))
  {
    CleanupDeviceD3D();
    OutputDebugStringW(L"failed to init d3d");
  }
  else
  {
    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    ::EnableWindow(hwnd, TRUE);  // enable keyboard input

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    auto oldContext = ImGui::GetCurrentContext();
    mImGuiContext = ImGui::CreateContext();

    //cc::log("Created context %p, overwriting %p", mImGuiContext, oldContext);

    // CreateContext doesn't actually set the context to the new one for multiple instances.
    auto contextToken = PushMyImGuiContext("PushMyImGuiContext: Initializing");

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // Do not force io.WantCaptureKeyboard: it is an output from Dear ImGui

    // Disable imgui.ini load/save
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);
  }
}

void VstEditor::Window_Close()
{
  //cc::LogScope ls ("Window_Close");
  auto contextToken = PushMyImGuiContext("PushMyImGuiContext: Window_Close()");

  ImGui_ImplDX9_Shutdown();
  ImGui_ImplWin32_Shutdown();

  //cc::log("Destrying imguicontext mImGuiContext=%p current_context=%p", mImGuiContext, ImGui::GetCurrentContext());

  ImGui::DestroyContext();
  mImGuiContext = nullptr;

  //cc::log(" -> and now context=%p", ImGui::GetCurrentContext());

  CleanupDeviceD3D();

  DestroyWindow(mCurrentWindow);
  mCurrentWindow = nullptr;
  mParentWindow = nullptr;
}

bool VstEditor::CreateDeviceD3D(HWND hWnd)
{
  if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
    return false;

  // Create the D3DDevice
  ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
  g_d3dpp.Windowed = TRUE;
  g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  g_d3dpp.BackBufferFormat =
      D3DFMT_UNKNOWN;  // Need to use an explicit format with alpha if needing per-pixel alpha composition.
  g_d3dpp.EnableAutoDepthStencil = TRUE;
  g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
  //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
  g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;  // Present without vsync, maximum unthrottled framerate
  if (g_pD3D->CreateDevice(
          D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
    return false;

  return true;
}

void VstEditor::CleanupDeviceD3D()
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

void VstEditor::ResetDevice()
{
  auto contextToken = PushMyImGuiContext("PushMyImGuiContext: ResetDevice()");

  ImGui_ImplDX9_InvalidateDeviceObjects();
  HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
  if (hr == D3DERR_INVALIDCALL)
    IM_ASSERT(0);
  ImGui_ImplDX9_CreateDeviceObjects();
}

LRESULT WINAPI VstEditor::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTCHARS;
    case WM_MOUSEACTIVATE:
      SetFocus(hWnd);
      return MA_ACTIVATE;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      if (GetFocus() != hWnd)
      {
        SetFocus(hWnd);
      }
      break;
  }

  if (mImGuiContext)
  {
    auto contextToken = PushMyImGuiContext("PushMyImGuiContext: ImGui_ImplWin32_WndProcHandler");
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
      return true;
    }
  }

  switch (msg)
  {
    case WM_SETFOCUS:
      if (mImGuiContext)
      {
        auto contextToken = PushMyImGuiContext("PushMyImGuiContext: WM_SETFOCUS");
        ImGui::GetIO().AddFocusEvent(true);
      }
      return 0;
    case WM_KILLFOCUS:
      if (mImGuiContext)
      {
        auto contextToken = PushMyImGuiContext("PushMyImGuiContext: WM_KILLFOCUS");
        ImGui::GetIO().AddFocusEvent(false);
      }
      return 0;
    case WM_SIZE:
      if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
      {
        g_d3dpp.BackBufferWidth = LOWORD(lParam);
        g_d3dpp.BackBufferHeight = HIWORD(lParam);
        ResetDevice();
      }
      return 0;
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU)  // Disable ALT application menu
        return 0;
      break;
    case WM_TIMER:
      if (!gIsRendering)
      {
        gIsRendering = true;
        ImguiPresent();
        gIsRendering = false;
      }
      return 0;
  }

  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT WINAPI VstEditor::gWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  VstEditor* pThis = (VstEditor*)GetPropW(hWnd, L"WSGuiInstance");
  if (msg == WM_CREATE)
  {
    auto pcs = (CREATESTRUCTW*)lParam;
    SetPropW(hWnd, L"WSGuiInstance", pcs->lpCreateParams);

    // reaper uses this weird magic value to specify keyboard input should be sent to the window. WHAT
    // https://forum.cockos.com/showthread.php?t=116894
    ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0xdeadf00b);
    ::SetFocus(hWnd);

    return TRUE;
  }
  if (!pThis)
  {
    return DefWindowProcW(hWnd, msg, wParam, lParam);
  }
  return pThis->WndProc(hWnd, msg, wParam, lParam);
}

struct HistMarker
{
  double value;
  ImU32 color;
  float thickness;
};

// Returns hovered bin index or -1; also draws highlights for primary/secondary, and markers.
static int RenderHistogramBars(const PerfHistogramSnapshot& h,
                               const ImVec2& size,
                               ImU32 color,
                               int highlightBinPrimary,
                               int highlightBinSecondary,
                               float valueScale,
                               const char* unitSuffix,
                               const HistMarker* markers = nullptr,
                               int markerCount = 0)
{
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImVec2 p0 = ImGui::GetCursorScreenPos();
  ImVec2 p1 = p0 + size;
  ImGui::RenderFrame(p0, p1, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f);

  int hovered = -1;
  if (h.total == 0)
  {
    ImGui::Dummy(size);
    return hovered;
  }

  int maxBin = 1;
  for (size_t i = 0; i < PerfHistogramSnapshot::kBins; ++i)
    maxBin = (int)std::max((size_t)maxBin, (size_t)h.bins[i]);

  float binW = size.x / (float)PerfHistogramSnapshot::kBins;
  const float pad = 2.0f;
  const double range = (h.max - h.min);

  for (int i = 0; i < (int)PerfHistogramSnapshot::kBins; ++i)
  {
    float x0 = p0.x + i * binW;
    float x1 = x0 + binW - 1.0f;
    float t = (float)h.bins[i] / (float)maxBin;
    float barH = t * (size.y - pad * 2.0f);
    ImVec2 br0 = ImVec2(x0, p1.y - barH - pad);
    ImVec2 br1 = ImVec2(x1, p1.y - pad);

    ImU32 col = color;
    if (i == highlightBinSecondary)
    {
      col = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered);
      dl->AddRect(ImVec2(x0, p0.y + pad), ImVec2(x1, p1.y - pad), col, 2.0f, 0, 2.0f);
    }
    if (i == highlightBinPrimary)
    {
      col = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered);
      dl->AddRectFilled(ImVec2(x0, p0.y + pad), ImVec2(x1, p1.y - pad), ImGui::GetColorU32(ImGuiCol_FrameBg), 1.5f);
    }
    dl->AddRectFilled(br0, br1, col, 1.5f);

    // Hover: use full column rect for easier hit
    ImRect hitRect(ImVec2(x0, p0.y + pad), ImVec2(x1, p1.y - pad));
    if (ImGui::IsMouseHoveringRect(hitRect.Min, hitRect.Max))
    {
      hovered = i;
    }
  }

  // Markers: draw vertical lines for key metrics
  if (markers && markerCount > 0 && range > 0.0)
  {
    for (int i = 0; i < markerCount; ++i)
    {
      double v = markers[i].value;
      float t = (float)((v - h.min) / range);
      float x = p0.x + ImClamp(t, 0.0f, 1.0f) * size.x;
      dl->AddLine(ImVec2(x, p0.y + pad), ImVec2(x, p1.y - pad), markers[i].color, markers[i].thickness);
    }
  }

  // Tooltip for hovered bin (center + width labeling)
  if (hovered >= 0)
  {
    double center = (h.min + h.binWidth * (0.5 + hovered));
    double width = h.binWidth;
    int count = h.bins[(size_t)hovered];
    float pct = (h.total > 0) ? (100.0f * (float)count / (float)h.total) : 0.0f;

    ImGui::BeginTooltip();
    ImGui::Text("%s%.3f +/- %.3f%s",
                unitSuffix ? unitSuffix : "",
                (float)(center * valueScale),
                (float)(0.5 * width * valueScale),
                unitSuffix ? unitSuffix : "");
    ImGui::Text("Count: %d (%.1f%%)", count, pct);
    ImGui::EndTooltip();
  }

  ImGui::Dummy(size);
  return hovered;
}

static int ComputeHighlightBin(const PerfHistogramSnapshot& h, double targetValue)
{
  if (h.total == 0)
    return -1;
  if (h.max <= h.min)
    return (int)(PerfHistogramSnapshot::kBins / 2);
  double t = (targetValue - h.min) / (h.max - h.min);
  int bin = (int)std::floor(t * (double)(PerfHistogramSnapshot::kBins - 1));
  if (bin < 0)
    bin = 0;
  if (bin >= (int)PerfHistogramSnapshot::kBins)
    bin = (int)PerfHistogramSnapshot::kBins - 1;
  return bin;
}

static void RenderSparkline(const PerfSeriesSnapshot& s, const ImVec2& size, ImU32 col)
{
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImVec2 p0 = ImGui::GetCursorScreenPos();
  ImVec2 p1 = p0 + size;
  ImGui::RenderFrame(p0, p1, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f);
  if (s.values.empty())
  {
    ImGui::Dummy(size);
    return;
  }

  const float minv = s.min;
  const float maxv = s.max;
  const float range = (maxv > minv) ? (maxv - minv) : 1.0f;
  const int n = (int)s.values.size();
  const float dx = (n > 1) ? (size.x / (n - 1)) : size.x;
  ImVec2 prev;
  for (int i = 0; i < n; ++i)
  {
    float t = (s.values[(size_t)i] - minv) / range;
    float x = p0.x + i * dx;
    float y = p1.y - 2.0f - t * (size.y - 4.0f);
    ImVec2 cur(x, y);
    if (i > 0)
      dl->AddLine(prev, cur, col, 1.5f);
    prev = cur;
  }
  ImGui::Dummy(size);
}

void VstEditor::ImguiPresent()
{
  auto contextToken = PushMyImGuiContext("PushMyImGuiContext: ImguiPresent()");

  ImGui_ImplDX9_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  // Drain perf samples from audio thread for UI stats
  auto* fx = GetEffectX();

  fx->Perf_DrainToUiThread();

  ImGui::GetStyle().FrameRounding = 3.0f;
  ImGui::GetStyle().DisabledAlpha = .2f;

  auto& io = ImGui::GetIO();

  ImGui::SetNextWindowPos(ImVec2{0, 0});
  ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Once);

  ImGui::Begin("##main",
               0,
               ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);


  if (ImGui::BeginMenuBar())
  {
    PopulateMenuBar();

    // Read perf stats snapshot for display
    auto usageStats = fx->Perf_GetCPUUsageStats();
    auto cpsStats = fx->Perf_GetCyclesPerSampleStats();
    
    auto* fx = GetEffectX();
    ImGui::Checkbox("##perfwindow", &fx->mShowingPerformanceWindow);

    ImGui::TextColored(ImColor{.5f, .5f, .5f},
                       "%.1f FPS    CPU: %.1f%%    Cyc/Spl: %.0f    %s",
                       ImGui::GetIO().Framerate,
                       (float)(usageStats.p50 * 100.0),
                       (float)(cpsStats.p50),
                       this->GetMenuBarStatusText().c_str());

    ImGui::EndMenuBar();
  }

  if (showingParamExplorer)
  {
    mParamExplorer.Render();
  }

  if (fx->mShowingPerformanceWindow)
  {
    // Ensure Perf window is not collapsed and cannot be collapsed
    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
    if (ImGui::Begin("Perf", &fx->mShowingPerformanceWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
    {
      auto* fx = GetEffectX();
      auto usageStats = fx->Perf_GetCPUUsageStats();
      auto cpsStats = fx->Perf_GetCyclesPerSampleStats();
      auto hUsage = fx->Perf_GetCPUUsageHistogram();
      auto hCps = fx->Perf_GetCyclesPerSampleHistogram();

      // Colors for markers
      ImU32 colP95 = ImColor(255, 170, 0);
      ImU32 colMode = ImColor(64, 200, 255);
      ImU32 colTrim = ImColor(120, 220, 120);
      ImU32 colMAD = ImColor(200, 120, 255);
      ImU32 colMed = ImColor(180, 180, 180);

      // CPU histogram with markers
      ImGui::Text("CPU load histogram (window): min=%.2f%% max=%.2f%%",
                  (float)(hUsage.min * 100.0),
                  (float)(hUsage.max * 100.0));
      int hiBinUsage = ComputeHighlightBin(hUsage, usageStats.p95);
      int modeBinUsage = hUsage.modeIndex;
      HistMarker markersUsage[] = {
          {usageStats.p95, colP95, 2.0f},
          {hUsage.min + hUsage.binWidth * (hUsage.modeIndex + 0.5), colMode, 2.0f},
          {usageStats.trimmedMean10, colTrim, 2.0f},
          {usageStats.madClippedMean, colMAD, 2.0f},
          {usageStats.p50, colMed, 1.5f},
      };
      (void)RenderHistogramBars(hUsage,
                                ImVec2(640, 200),
                                ImGui::GetColorU32(ImGuiCol_PlotHistogram),
                                hiBinUsage,
                                modeBinUsage,
                                100.0f,
                                "%",
                                markersUsage,
                                (int)(sizeof(markersUsage) / sizeof(markersUsage[0])));

      // CPU sparkline (last N)
      auto seriesUsage = fx->Perf_GetCPUUsageSeries(0);
      RenderSparkline(seriesUsage, ImVec2(640, 100), ImGui::GetColorU32(ImGuiCol_PlotLines));

      // CPU textual guide, color-coded
      double modeCenterUsage = (hUsage.total > 0 && hUsage.modeIndex >= 0)
                                   ? (hUsage.min + hUsage.binWidth * (hUsage.modeIndex + 0.5))
                                   : 0.0;
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colP95), "p95=%.2f%%", (float)(usageStats.p95 * 100.0));
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colMode), "mode=%.2f%%", (float)(modeCenterUsage * 100.0));
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colTrim),
                         "trim10=%.2f%%",
                         (float)(usageStats.trimmedMean10 * 100.0));
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colMAD),
                         "madMean=%.2f%%",
                         (float)(usageStats.madClippedMean * 100.0));
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colMed), "median=%.2f%%", (float)(usageStats.p50 * 100.0));

      // CPS histogram with markers
      ImGui::Separator();
      ImGui::Text("Cycles/sample histogram (window): min=%.0f max=%.0f", (float)hCps.min, (float)hCps.max);
      int hiBinCps = ComputeHighlightBin(hCps, cpsStats.p95);
      int modeBinCps = hCps.modeIndex;
      HistMarker markersCps[] = {
          {cpsStats.p95, colP95, 2.0f},
          {hCps.min + hCps.binWidth * (hCps.modeIndex + 0.5), colMode, 2.0f},
          {cpsStats.trimmedMean10, colTrim, 2.0f},
          {cpsStats.madClippedMean, colMAD, 2.0f},
          {cpsStats.p50, colMed, 1.5f},
      };
      (void)RenderHistogramBars(hCps,
                                ImVec2(640, 200),
                                ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered),
                                hiBinCps,
                                modeBinCps,
                                1.0f,
                                "",
                                markersCps,
                                (int)(sizeof(markersCps) / sizeof(markersCps[0])));

      // CPS sparkline (last N)
      auto seriesCps = fx->Perf_GetCyclesPerSampleSeries(0);
      RenderSparkline(seriesCps, ImVec2(640, 100), ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered));

      double modeCenterCps = (hCps.total > 0 && hCps.modeIndex >= 0)
                                 ? (hCps.min + hCps.binWidth * (hCps.modeIndex + 0.5))
                                 : 0.0;
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colP95), "p95=%.0f", (float)(cpsStats.p95));
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colMode), "mode=%.0f", (float)(modeCenterCps));
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colTrim), "trim10=%.0f", (float)(cpsStats.trimmedMean10));
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colMAD), "madMean=%.0f", (float)(cpsStats.madClippedMean));
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colMed), "median=%.0f", (float)(cpsStats.p50));
    }
    ImGui::End();
  }

  this->renderImgui();

  ImGui::End();

  if (showingDemo)
  {
    ImGui::ShowDemoWindow(&showingDemo);
  }

  // Rendering
  ImGui::EndFrame();
  g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
  g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
  static constexpr ImVec4 clear_color{};  // ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  static constexpr D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f),
                                                         (int)(clear_color.y * clear_color.w * 255.0f),
                                                         (int)(clear_color.z * clear_color.w * 255.0f),
                                                         (int)(clear_color.w * 255.0f));
  g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
  if (g_pd3dDevice->BeginScene() >= 0)
  {
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    g_pd3dDevice->EndScene();
  }
  HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

  // Handle loss of D3D9 device
  if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
    ResetDevice();
}

}  // namespace WaveSabreVstLib
