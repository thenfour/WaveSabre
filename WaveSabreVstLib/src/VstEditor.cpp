
#include <WaveSabreVstLib/VstEditor.h>
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

#include <sstream>

using namespace std;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void* hInstance;

static std::atomic_bool gIsRendering = false;

namespace WaveSabreVstLib
{
	VstEditor::VstEditor(AudioEffect *audioEffect, int width, int height)
		: AEffEditor(audioEffect)
	{
		audioEffect->setEditor(this);
		auto r = CoInitialize(0);

		mDefaultRect.right = width;
		mDefaultRect.bottom = height;
		setKnobMode(2/*kLinearMode*/);
	}

	VstEditor::~VstEditor()
	{
		CoUninitialize();
	}

	void VstEditor::Window_Open(HWND parentWindow)
	{
		//cc::LogScope ls("Window_Open");
		static constexpr wchar_t const* const className = L"wavesabre vst";
		WNDCLASSEXW wc = { sizeof(wc), CS_DBLCLKS, gWndProc, 0L, 0L, (HINSTANCE)::hInstance, NULL, NULL, NULL, NULL, className, NULL };
		::RegisterClassExW(&wc);

		HWND hwnd = CreateWindowExW(0, className, nullptr, WS_CHILD,
			0, 0, (mDefaultRect.right - mDefaultRect.left), (mDefaultRect.bottom - mDefaultRect.top),
			parentWindow, (HMENU)1, (HINSTANCE)::hInstance, this);

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
		} else {

			// Show the window
			::ShowWindow(hwnd, SW_SHOWDEFAULT);
			::UpdateWindow(hwnd);
			::EnableWindow(hwnd, TRUE); // enable keyboard input

			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			auto oldContext = ImGui::GetCurrentContext();
			mImGuiContext = ImGui::CreateContext();

			//cc::log("Created context %p, overwriting %p", mImGuiContext, oldContext);

			// CreateContext doesn't actually set the context to the new one for multiple instances.
			auto contextToken = PushMyImGuiContext("PushMyImGuiContext: Initializing");

			ImGuiIO& io = ImGui::GetIO();
			(void)io;
			io.WantCaptureKeyboard = true;// ();

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
		g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
		g_d3dpp.EnableAutoDepthStencil = TRUE;
		g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
		g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
		if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
			return false;

		return true;
	}

	void VstEditor::CleanupDeviceD3D()
	{
		if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
		if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
	}

	void VstEditor::ResetDevice() {
		auto contextToken = PushMyImGuiContext("PushMyImGuiContext: ResetDevice()");

		ImGui_ImplDX9_InvalidateDeviceObjects();
		HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
		if (hr == D3DERR_INVALIDCALL)
			IM_ASSERT(0);
		ImGui_ImplDX9_CreateDeviceObjects();
	}

	LRESULT WINAPI VstEditor::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS;
		}

		if (mImGuiContext) {

			auto contextToken = PushMyImGuiContext("PushMyImGuiContext: ImGui_ImplWin32_WndProcHandler");
			if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
				return true;
			}
		}

		switch (msg)
		{
		case WM_SIZE:
			if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
			{
				g_d3dpp.BackBufferWidth = LOWORD(lParam);
				g_d3dpp.BackBufferHeight = HIWORD(lParam);
				ResetDevice();
			}
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_TIMER:
			if (!gIsRendering) {
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
		if (msg == WM_CREATE) {
			auto pcs = (CREATESTRUCTW*)lParam;
			SetPropW(hWnd, L"WSGuiInstance", pcs->lpCreateParams);

			// reaper uses this weird magic value to specify keyboard input should be sent to the window. WHAT
			// https://forum.cockos.com/showthread.php?t=116894
			::SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0xdeadf00b);

			return TRUE;
		}
		if (!pThis) {
			return DefWindowProcW(hWnd, msg, wParam, lParam);
		}
		return pThis->WndProc(hWnd, msg, wParam, lParam);
	}

	void VstEditor::ImguiPresent()
	{
		auto contextToken = PushMyImGuiContext("PushMyImGuiContext: ImguiPresent()");

		// make sure our child window is the same size as the one given to us.
		// i guess this would be necessary to support when the user resizes the plugin window to expand to fill it.
		// however on Renoise, it means covering up host controls like "Enable keyboard" checkbox etc. don't clobber things the host is not expecting.
		//RECT rcParent, rcWnd;
		//GetWindowRect(mParentWindow, &rcParent);
		//GetWindowRect(mCurrentWindow, &rcWnd);

		//auto parentX = (rcParent.right - rcParent.left);
		//auto childX = (rcWnd.right - rcWnd.left);
		//auto parentY = (rcParent.bottom - rcParent.top);
		//auto childY = (rcWnd.bottom - rcWnd.top);

		//if ((parentX != childX) || (parentY != childY))
		//{
		//	SetWindowPos(mCurrentWindow, 0, 0, 0, parentX, parentY, 0);
		//	// skip this frame now that window pos has changed it's just safer to not have to consider this case.
		//	return;
		//}

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::GetStyle().FrameRounding = 3.0f;

		auto& io = ImGui::GetIO();

		ImGui::SetNextWindowPos(ImVec2{ 0, 0 });
		ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Once);

		ImGui::Begin("##main", 0, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

		if (ImGui::BeginMenuBar()) {

#ifdef _DEBUG
			if (ImGui::BeginMenu("Wavesabre")) {
				if (ImGui::MenuItem("Toggle ImGui demo window", nullptr, showingDemo)) {
					showingDemo = !showingDemo;
				}
				if (ImGui::MenuItem("Toggle param explorer", nullptr, showingParamExplorer)) {
					showingParamExplorer = !showingParamExplorer;
				}
				ImGui::EndMenu();
			}
#endif
			PopulateMenuBar();

			//char effectName[kVstMaxEffectNameLen * 2 + 50];
			//GetEffectX()->getEffectName(effectName);
			//char title[200];
			// i don't really know if it's kosher to do this in a menu bar.
			ImGui::TextColored(ImColor{ .5f, .5f, .5f }, "%.1f FPS, CPU: %.2f %s", ImGui::GetIO().Framerate, GetEffectX()->GetCPUUsage01() * 100, this->GetMenuBarStatusText().c_str());

			ImGui::EndMenuBar();
		}

		if (showingParamExplorer) {
			mParamExplorer.Render();
		}

		this->renderImgui();

		ImGui::End();

		if (showingDemo) {
			ImGui::ShowDemoWindow(&showingDemo);
		}

		// Rendering
		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		static constexpr ImVec4 clear_color{};// ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		static constexpr D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
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

}
