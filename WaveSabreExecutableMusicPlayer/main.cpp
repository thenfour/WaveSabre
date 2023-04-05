// song renders in the background
// user can choose to save at any time; the file will be written when rendering is done.
// if rendering is too slow to comply with prerender constraints, give the user the option to begin rather than just beginning.

#include <Windows.h>
#include <string.h>
#include <Commctrl.h>
#include <Commdlg.h>
#pragma comment(lib, "Comdlg32.lib")

#include <WaveSabreCore.h>
#include <WaveSabrePlayerLib.h>
using namespace WaveSabrePlayerLib;

#include "utils.hpp"
#include "config.hpp"

#include "player.hpp"
#include "waveformgen.hpp"
#include "wavwriter.hpp"

char gFilename[MAX_PATH] = { 0 };// = DEFAULT_FILENAME;
char gSavedToFilename[MAX_PATH] = { 0 }; // gets populated upon successful save.

char* gWindowText = nullptr;
HWND hMain;

Renderer* gpRenderer = nullptr;
Player* gpPlayer = nullptr;
WaveformGen* gpWaveformGen = nullptr;

// position between min(precalctime_max, render_progress)
int32_t gPrecalcProgressPercent = 0;

void UpdateStatusText()
{
    auto lock = gpRenderer->gCritsec.Enter();

#ifdef WS_EXEPLAYER_RELEASE_FEATURES

    static constexpr char format[] =
        "%s"
        "Press F7 to save.\r\n"
        "  %s\r\n"
        "\r\n"
        "%s\r\n"
        ;

    int32_t renderPercent = gpRenderer->gSongRendered.AsPercentOf(gpRenderer->gSongLength);

    // actual precalc time will be the sooner of: [max_precalc_allowed, complete render]
    gPrecalcProgressPercent = std::max(renderPercent, gpRenderer->gRenderTime.AsPercentOf(WSTime::FromMilliseconds(gMaxPrecalcMilliseconds)));

    char saveIndicatorText[1000] = { 0 };
    if (gSavedToFilename[0]) {
        sprintf(saveIndicatorText, "Saved to \"%s\"", gSavedToFilename);
    }
    else if (gFilename[0]) {
        sprintf(saveIndicatorText, "When finished rendering, will be saved to \"%s\"", gFilename);
    }

    sprintf(gWindowText, format,
        TEXT_INTRO,
        saveIndicatorText,
        gPrecalcProgressPercent >= 100 ? "Precalc done: Press F5 to play" : "Wait for precalc before playing..."
        );

#else
    static constexpr char format[] =
        "%s"
        "F5: Play (while playing, click to seek)\r\n"
        "F6: Stop\r\n"
        "F7: Write .WAV file\r\n"
        "  %s\r\n"
        "\r\n"
        //"Song length: %d:%d.%d\r\n"
        "Render progress %d%% (%d.%02dx real-time) using %d threads\r\n"
        //"- song rendered: %d:%02d.%d\r\n"
        //"- song remaining: %d:%02d.%d\r\n"
        "Render time elapsed: %d:%02d.%d (est remaining: %d:%02d.%d) (est total:  %d:%02d.%d)\r\n"
        "\r\n"
        "Time remaining before you can safely play the whole track: %d:%02d.%d (total precalc time: %d:%02d.%d)\r\n"
        // TODO: wav writing status & destination
        ;

    //WSTime renderTime = WSTime::FromMilliseconds(GetTickCount() - renderingStartedTick);
    //if (!gpRenderer->IsRenderingComplete()) {
    //    gpRenderer->gRenderTime.SetMilliseconds(GetTickCount() - gpRenderer->renderingStartedTick);
    //}
    int32_t renderPercent = gpRenderer->gSongRendered.AsPercentOf(gpRenderer->gSongLength);

    //const char* renderStatus = "";
    //if (gpRenderer && isRenderingComplete) {
    //    renderStatus = "Done";
    //}
    //else if (gpRenderer) {
    //    renderStatus = "Rendering";
    //}

    //::SendMessageA(hRenderProgress, PBM_SETPOS, (WPARAM)(renderPercent), 0);

    auto renderRate = gpRenderer->gSongRendered.AsRateOf_x100(gpRenderer->gRenderTime);
    auto songYetToRender = gpRenderer->gSongLength - gpRenderer->gSongRendered;
    auto remainingRenderTime = songYetToRender.DividedByRate_x100(renderRate);
    auto estTotalRenderTime = remainingRenderTime + gpRenderer->gRenderTime;

    // now let's calculate some buffering stuff. well this is going to be unreliable because the song doesn't render at the same rate throughout.
    // we want to show "how long until the song should be playable without buffering?"
    // basically it's similar to saying "at what point does the render-time-remaining fall below play-time-remaining?
    auto totalPrecalcTime = estTotalRenderTime - gpRenderer->gSongLength;
    auto remainingPrecalcTime = totalPrecalcTime - gpRenderer->gRenderTime; // subtract how much you've already waited.
    remainingPrecalcTime.Max0();
    totalPrecalcTime.Max0();

    gPrecalcProgressPercent = std::max(renderPercent, gpRenderer->gRenderTime.AsPercentOf(WSTime::FromMilliseconds(gMaxPrecalcMilliseconds)));

    char saveIndicatorText[1000] = { 0 };
    if (gSavedToFilename[0]) {
        sprintf(saveIndicatorText, "Saved to \"%s\"", gSavedToFilename);
    }
    else if (gFilename[0]) {
        sprintf(saveIndicatorText, "When finished rendering, will be saved to \"%s\"", gFilename);
    }

    sprintf(gWindowText, format,
        //gSongLength.GetMinutes(), gSongLength.GetSecondsOfMinute(), gSongLength.GetTenthsOfSecondsOfSeconds(),
        //renderStatus,
        TEXT_INTRO,
        saveIndicatorText,
        renderPercent,
        renderRate / 100, renderRate % 100,
        gpRenderer->mProcessorCount,
        gpRenderer->gRenderTime.GetMinutes(), gpRenderer->gRenderTime.GetSecondsOfMinute(), gpRenderer->gRenderTime.GetTenthsOfSecondsOfSeconds(),
        //gSongRendered.GetMinutes(), gSongRendered.GetSecondsOfMinute(), gSongRendered.GetTenthsOfSecondsOfSeconds(),
        //songYetToRender.GetMinutes(), songYetToRender.GetSecondsOfMinute(), songYetToRender.GetTenthsOfSecondsOfSeconds(),
        remainingRenderTime.GetMinutes(), remainingRenderTime.GetSecondsOfMinute(), remainingRenderTime.GetTenthsOfSecondsOfSeconds(),
        estTotalRenderTime.GetMinutes(), estTotalRenderTime.GetSecondsOfMinute(), estTotalRenderTime.GetTenthsOfSecondsOfSeconds(),
        remainingPrecalcTime.GetMinutes(), remainingPrecalcTime.GetSecondsOfMinute(), remainingPrecalcTime.GetTenthsOfSecondsOfSeconds(),
        totalPrecalcTime.GetMinutes(), totalPrecalcTime.GetSecondsOfMinute(), totalPrecalcTime.GetTenthsOfSecondsOfSeconds()
    );


#endif

}

void WriteWave()
{
    WaveWriter::WriteWAV(gFilename, *gpRenderer);
    strcpy(gSavedToFilename, gFilename);
    gFilename[0] = 0;
}

void handleSave() {
    OPENFILENAMEA o = {
        sizeof(o),//DWORD        lStructSize;
        hMain,//HWND         hwndOwner;
        0,//HINSTANCE    hInstance;
        "WAV files (*.wav)\0*.wav\0",//LPCSTR       lpstrFilter;
        0,0,0,
        gFilename,//LPSTR        lpstrFile;
        sizeof(gFilename) / sizeof(gFilename[0]),//DWORD        nMaxFile;
        0,0,0,
        "",//LPCSTR       lpstrTitle;
        0,0,0,// flags, fileoffset, nextension
        "wav",//LPCSTR       lpstrDefExt;
        0
    };
    if (::GetSaveFileNameA(&o)) {
        if (gpRenderer->GetRenderStatus() == Renderer::RenderStatus::Done) {
            WriteWave();
        }
    }
}

#ifndef SMALL_PAINT
void RenderWaveform_Fancy(GdiDeviceContext& dc)
{
    auto lock = gpRenderer->gCritsec.Enter(); // don't give waveformgen its own critsec for 1) complexity (lock hierarchies!) and 2) code size.

    dc.HatchFill(gpWaveformGen->mRect, gColorScheme.WaveformBackground, gColorScheme.WaveformBackground);
    const auto midY = gpWaveformGen->mRect.GetMidY();
    const auto left = gpWaveformGen->mRect.GetLeft();
    Point renderCursorP1{ gpWaveformGen->mRect.GetLeft() + gpWaveformGen->mProcessedWidth, gpWaveformGen->mRect.GetTop() };
    Point renderCursorP2{ renderCursorP1.GetX(), gpWaveformGen->mRect.GetBottom() };
    for (int i = 0; i < gpWaveformGen->mProcessedWidth; ++i) {
        auto h = gpWaveformGen->mHeights[i];

        const Point p1{ left + i, midY - h.first };
        const Point p2{ p1.GetX(), midY + h.second };
        // distance to render cursor
        int distToRenderCursor = renderCursorP1.GetX() - p1.GetX();
        float t = float(distToRenderCursor) / gWaveformGradientMaxDistancePixels;
        t = WaveSabreCore::M7::math::clamp01(1.0f - t);
        t = WaveSabreCore::M7::math::modCurve_xN11_kN11(t, -0.95f);
        COLORREF g = RGB(
            WaveSabreCore::M7::math::lerp(GetRValue(gColorScheme.WaveformForeground), GetRValue(gColorScheme.RenderCursorColor), t),
            WaveSabreCore::M7::math::lerp(GetGValue(gColorScheme.WaveformForeground), GetGValue(gColorScheme.RenderCursorColor), t),
            WaveSabreCore::M7::math::lerp(GetBValue(gColorScheme.WaveformForeground), GetBValue(gColorScheme.RenderCursorColor), t)
            );
        
        dc.DrawLine(p1, p2, g);
    }
    dc.HatchFill(gpWaveformGen->GetUnprocessedRect(), gColorScheme.WaveformUnrenderedHatch1, gColorScheme.WaveformUnrenderedHatch2);
    
    Point midLineP1{ left, midY };
    Point midLineP2{ renderCursorP1.GetX(), midY };
    dc.DrawLine(midLineP1, midLineP2, gColorScheme.WaveformZeroLine);

    dc.DrawLine(renderCursorP1, renderCursorP2, gColorScheme.RenderCursorColor);

    if (gpPlayer->IsPlaying()) {
        auto playFrames = gpPlayer->gPlayTime.GetFrames();
        auto c = (playFrames >= gpRenderer->gSongRendered.GetFrames()) ? gColorScheme.PlayCursorBad : gColorScheme.PlayCursorGood;

        int playCursorX = MulDiv(playFrames, gpWaveformGen->mRect.GetWidth(), gpRenderer->gSongLength.GetFrames());
        static constexpr int gPlayCursorWidth = 4;
        Rect rc{ gpWaveformGen->mRect.GetLeft() + playCursorX - gPlayCursorWidth / 2, renderCursorP1.GetY(), gPlayCursorWidth, gpWaveformGen->mRect.GetHeight() };

        dc.HatchFill(rc, c, c);
    }
}
void handlePaint_Fancy()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hMain, &ps);

    // Double-buffering: create off-screen bitmap
    GdiDeviceContext dc{ CreateCompatibleDC(hdc) };
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(dc.mDC, hBitmap);

    HFONT hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    LOGFONT lf;
    GetObject(hFont, sizeof(LOGFONT), &lf);
    lf.lfHeight *= 2; // double the font height
    lf.lfWeight = 1000;
    HFONT hCustomFont = CreateFontIndirect(&lf); // create a new font based on the modified information

    HFONT hOldFont = (HFONT)SelectObject(dc.mDC, hCustomFont);
    RenderWaveform_Fancy(dc);

    dc.DrawText_(gWindowText, grcText, gColorScheme.TextColor, gColorScheme.TextShadowColor);

    if (gPrecalcProgressPercent < 100) {
        dc.HatchFill(grcPrecalcProgress, gColorScheme.PrecalcProgressBackground, gColorScheme.PrecalcProgressBackground);
        dc.HatchFill(grcPrecalcProgress.LeftAlignedShrink(gPrecalcProgressPercent), gColorScheme.PrecalcProgressForeground, gColorScheme.PrecalcProgressForeground);
        char sz[100];
        sprintf(sz, "Precalculating %d%%...", gPrecalcProgressPercent);
        dc.DrawText_(sz, grcPrecalcProgress, gColorScheme.PrecalcTextColor, gColorScheme.PrecalcTextShadowColor);
    }

    // present the back buffer
    BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, dc.mDC, 0, 0, SRCCOPY);

    SelectObject(dc.mDC, hOldFont);
    DeleteObject(hCustomFont);
    SelectObject(dc.mDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(dc.mDC);

    EndPaint(hMain, &ps);
}
#else 
void RenderWaveform_Basic(GdiDeviceContext& dc)
{
    auto lock = gpRenderer->gCritsec.Enter(); // don't give waveformgen its own critsec for 1) complexity (lock hierarchies!) and 2) code size.

    dc.HatchFill(gpWaveformGen->mRect, gColorScheme.WaveformBackground, gColorScheme.WaveformBackground);
    const auto midY = gpWaveformGen->mRect.GetMidY();
    const auto left = gpWaveformGen->mRect.GetLeft();
    Point renderCursorP1{ gpWaveformGen->mRect.GetLeft() + gpWaveformGen->mProcessedWidth, gpWaveformGen->mRect.GetTop() };
    Point renderCursorP2{ renderCursorP1.GetX(), gpWaveformGen->mRect.GetBottom() };
    for (int i = 0; i < gpWaveformGen->mProcessedWidth; ++i) {
        auto h = gpWaveformGen->mHeights[i];

        const Point p1{ left + i, midY - h };
        const Point p2{ p1.GetX(), midY + h };
        // distance to render cursor
        int distToRenderCursor = renderCursorP1.GetX() - p1.GetX();
        float t = float(distToRenderCursor) / gWaveformGradientMaxDistancePixels;
        t = WaveSabreCore::M7::math::clamp01(1.0f - t);
        t = WaveSabreCore::M7::math::modCurve_xN11_kN11(t, -0.95f);

        dc.DrawLine(p1, p2, gColorScheme.WaveformForeground);
    }
    dc.HatchFill(gpWaveformGen->GetUnprocessedRect(), gColorScheme.WaveformUnrenderedHatch1, gColorScheme.WaveformUnrenderedHatch2);

    //Point midLineP1{ left, midY };
    //Point midLineP2{ renderCursorP1.GetX(), midY };
    //dc.DrawLine(midLineP1, midLineP2, gColorScheme.WaveformZeroLine);

    //dc.DrawLine(renderCursorP1, renderCursorP2, gColorScheme.RenderCursorColor);

    if (gpPlayer->IsPlaying()) {
        auto playFrames = gpPlayer->gPlayTime.GetFrames();
        //auto c = (playFrames >= gpRenderer->gSongRendered.GetFrames()) ? gColorScheme.PlayCursorBad : gColorScheme.PlayCursorGood;

        int playCursorX = MulDiv(playFrames, gpWaveformGen->mRect.GetWidth(), gpRenderer->gSongLength.GetFrames());
        //static constexpr int gPlayCursorWidth = 4;
        Rect rc{ gpWaveformGen->mRect.GetLeft() + playCursorX, renderCursorP1.GetY(), 1, gpWaveformGen->mRect.GetHeight() };

        dc.HatchFill(rc, gColorScheme.PlayCursorGood, gColorScheme.PlayCursorGood);
    }
}
#endif

void handlePaint_Basic()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hMain, &ps);

    // Double-buffering: create off-screen bitmap
    GdiDeviceContext dc{ CreateCompatibleDC(hdc) };
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(dc.mDC, hBitmap);

    //HFONT hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    //LOGFONT lf;
    //GetObject(hFont, sizeof(LOGFONT), &lf);
    //lf.lfHeight *= 2; // double the font height
    //lf.lfWeight = 1000;
    //HFONT hCustomFont = CreateFontIndirect(&lf); // create a new font based on the modified information

    //HFONT hOldFont = (HFONT)SelectObject(dc.mDC, hCustomFont);

    RenderWaveform_Basic(dc);

    dc.DrawText_(gWindowText, grcText, gColorScheme.TextColor, gColorScheme.TextShadowColor);

    if (gPrecalcProgressPercent < 100) {
        dc.HatchFill(grcPrecalcProgress, gColorScheme.PrecalcProgressBackground, gColorScheme.PrecalcProgressBackground);
        dc.HatchFill(grcPrecalcProgress.LeftAlignedShrink(gPrecalcProgressPercent), gColorScheme.PrecalcProgressForeground, gColorScheme.PrecalcProgressForeground);
        char sz[100];
        sprintf(sz, "Precalculating %d%%...", gPrecalcProgressPercent);
        dc.DrawText_(sz, grcPrecalcProgress, gColorScheme.PrecalcTextColor, gColorScheme.PrecalcTextShadowColor);
    }

    // present the back buffer
    BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, dc.mDC, 0, 0, SRCCOPY);

    //SelectObject(dc.mDC, hOldFont);
    //DeleteObject(hCustomFont);
    SelectObject(dc.mDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(dc.mDC);

    EndPaint(hMain, &ps);
}



LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_RENDERINGCOMPLETE:
    {
        if (gFilename[0]) {
            WriteWave();
        }
        return 0;
    }
    case WM_TIMER: {
        UpdateStatusText();
        ::InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    //case WM_ERASEBKGND: // this is actually not required now that i'm handling WM_PAINT and double-buffer
    //{
    //    return TRUE; // reduce flickering
    //}
#ifdef WS_EXEPLAYER_DEBUG_FEATURES
    case WM_LBUTTONUP:
    {
        Point p{ LOWORD(lParam), HIWORD(lParam) };
        if (!grcWaveform.ContainsPoint(p)) {
            return 0;
        }
        uint64_t xPos = LOWORD(lParam) - grcWaveform.GetLeft();
        uint32_t ms = MulDiv(xPos, gpRenderer->gSongLength.GetMilliseconds(), grcWaveform.GetWidth());
        if (gpPlayer->IsPlaying()) {
            gpPlayer->PlayFrom(WSTime::FromMilliseconds(ms));
        }
        return 0;
    }
#endif
    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_F7:
            handleSave();
            return 0;
        case VK_F5:
#ifdef WS_EXEPLAYER_RELEASE_FEATURES
            if (gPrecalcProgressPercent >= 100) {
                gpPlayer->PlayFrom(WSTime::FromFrames(0));
            }
#else
            gpPlayer->PlayFrom(WSTime::FromFrames(0));
#endif
            return 0;
#ifdef WS_EXEPLAYER_DEBUG_FEATURES
        case VK_F6:
            gpPlayer->Reset();
            return 0;
        case VK_F8:
            //WaveSabrePlayerLib::gpGraphProfiler->Dump();
            return 0;
#endif
        }

        break;
    }
    case WM_PAINT:
    {
#ifdef SMALL_PAINT
        handlePaint_Basic();
#else
        handlePaint_Fancy();
#endif
        return 0;
    }
    case WM_CLOSE:
        ExitProcess(0); // avoid importing postquitmessage and having to release tons of stuff (bits bits bits)
        return 0;
    }

    // since we're completely hijacking the window don't call old proc.
    //return ::CallWindowProcA(oldProc, hwnd, msg, wParam, lParam);
    return ::DefWindowProcA(hwnd, msg, wParam, lParam);
}


#ifdef MIN_SIZE_REL
extern "C" int WinMainCRTStartup()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdline, int show)
#endif
{
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    gWindowText = new char[60000];
    gWindowText[0] = 0;

    hMain = ::CreateWindowExA(0, "EDIT", TEXT_INTRO, WS_VISIBLE | ES_READONLY | ES_MULTILINE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, grcWindow.GetWidth(), grcWindow.GetHeight(), 0, 0, 0, 0);

    auto oldProc = (WNDPROC)::SetWindowLongPtrA(hMain, GWLP_WNDPROC, (LONG_PTR)WindowProc);

    gpRenderer = new Renderer(hMain);
    gpPlayer = new Player(*gpRenderer);
    gpWaveformGen = new WaveformGen(grcWaveform, *gpRenderer);
    gpRenderer->Begin(gpWaveformGen);

    SetTimer(hMain, 0, gGeneralSleepPeriodMS, 0);

    MSG msg;
    while (true) {
        GetMessage(&msg, 0, 0, 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
