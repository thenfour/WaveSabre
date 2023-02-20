
#include <WaveSabreVstLib/VstEditor.h>
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

#include <sstream>

using namespace std;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void* hInstance;

namespace WaveSabreVstLib
{
	VstEditor::VstEditor(AudioEffect *audioEffect, int width, int height)
		: AEffEditor(audioEffect)
	{
		audioEffect->setEditor(this);
		auto r = CoInitialize(0);

		mDefaultRect.right = width;
		mDefaultRect.bottom = height;
		setKnobMode(kLinearMode);
	}

	VstEditor::~VstEditor()
	{
		CoUninitialize();
	}

	void VstEditor::Window_Open(HWND parentWindow)
	{
		static constexpr wchar_t const* const className = L"wavesabre vst";
		WNDCLASSEXW wc = { sizeof(wc), CS_HREDRAW | CS_VREDRAW, gWndProc, 0L, 0L, (HINSTANCE)::hInstance, NULL, NULL, NULL, NULL, className, NULL };
		::RegisterClassExW(&wc);

		HWND hwnd = CreateWindowExW(0, className, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP,
			0, 0, mDefaultRect.right - mDefaultRect.left, mDefaultRect.bottom - mDefaultRect.top,
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
			ImGui::CreateContext();
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
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

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
		ImGui_ImplDX9_InvalidateDeviceObjects();
		HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
		if (hr == D3DERR_INVALIDCALL)
			IM_ASSERT(0);
		ImGui_ImplDX9_CreateDeviceObjects();
	}

	LRESULT WINAPI VstEditor::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// messages to ignore. we will handle character keys via the VST key notifications.
		switch (msg)
		{
		case WM_SYSKEYDOWN:
		{
			//char b[200] = { 0 };
			//std::sprintf(b, "windows WM_SYSKEYDOWN : %d", (int)wParam);
			//OutputDebugStringA(b);
			//return 0;
			break;
		}
		case WM_SYSKEYUP:
		{
			//char b[200] = { 0 };
			//std::sprintf(b, "windows WM_SYSKEYUP : %d", (int)wParam);
			//OutputDebugStringA(b);
			//return 0;
			break;
		}
		case WM_KEYDOWN:
		{
			//char b[200] = { 0 };
			//std::sprintf(b, "windows WM_KEYDOWN : %d", (int)wParam);
			//OutputDebugStringA(b);
			//return 0;
			break;
		}
		case WM_KEYUP:
		{
			//char b[200] = { 0 };
			//std::sprintf(b, "windows WM_KEYUP : %d", (int)wParam);
			//OutputDebugStringA(b);
			//return 0;
			break;
		}
		case WM_CHAR:
		{
			//char b[200] = { 0 };
			//std::sprintf(b, "windows WM_CHAR: %d", (int)wParam);
			//OutputDebugStringA(b);
			return ::DefWindowProcW(hWnd, msg, wParam, lParam);
		}
		}

		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
			return true;
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
			if (!mIsRendering) {
				mIsRendering = true;
				ImguiPresent();
				mIsRendering = false;
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
			return TRUE;
		}
		if (!pThis) {
			return DefWindowProcW(hWnd, msg, wParam, lParam);
		}
		return pThis->WndProc(hWnd, msg, wParam, lParam);
	}

	void VstEditor::ImguiPresent()
	{
		// make sure our child window is the same size as the one given to us.
		RECT rcParent, rcWnd;
		GetWindowRect(mParentWindow, &rcParent);
		GetWindowRect(mCurrentWindow, &rcWnd);

		auto parentX = (rcParent.right - rcParent.left);
		auto childX = (rcWnd.right - rcWnd.left);
		auto parentY = (rcParent.bottom - rcParent.top);
		auto childY = (rcWnd.bottom - rcWnd.top);
		if ((parentX != childX) || (parentY != childY))
		{
			SetWindowPos(mCurrentWindow, 0, 0, 0, parentX, parentY, 0);
			// skip this frame now that window pos has changed it's just safer to not have to consider this case.
			return;
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::GetStyle().FrameRounding = 3.0f;

		auto& io = ImGui::GetIO();

		ImGui::SetNextWindowPos(ImVec2{ 0, 0 });
		ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Once);

		char title[500];
		GetEffectX()->getEffectName(title);

		ImGui::Begin(title, 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

		if (ImGui::Button("Demo"))
		{
			showingDemo = !showingDemo;
		}

		ImGui::SameLine();
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

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
		static constexpr ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
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
