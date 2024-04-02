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
using namespace WSPlayerApp;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define WS_EXEPLAYER_RELEASE_FEATURES
//#define FANCY_PAINT

// Fancy paint has waveform view and more debug info displayed.
#ifndef WS_EXEPLAYER_RELEASE_FEATURES
#define FANCY_PAINT
#endif

// For compo release, don't allow playing before precalc is done. To avoid possibility of underrun.
// For debugging it's useful though.
#ifdef WS_EXEPLAYER_RELEASE_FEATURES
//#define ALLOW_EARLY_PLAY
#else
#define ALLOW_EARLY_PLAY
#endif

#ifndef WS_EXEPLAYER_RELEASE_FEATURES
#define HIGH_PROCESS_PRIORITY
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "rendered.hpp"
#define TEXT_INTRO "\"Dinette\"\r\nby tenfour/RBBS for Revision 2024\r\n\r\n"


char gFilename[MAX_PATH] = { 0 };
char gSavedToFilename[MAX_PATH] = { 0 }; // gets populated upon successful save.

char* gWindowText = nullptr;
HWND hMain;

WSPlayerApp::Renderer* gpRenderer = nullptr;
WSPlayerApp::WaveOutPlayer* gpPlayer = nullptr;

#ifdef FANCY_PAINT
WaveformGen* gpWaveformGen = nullptr;
using GdiDeviceContext = GdiDeviceContextFancy;
auto& gTheme = gThemeFancy;
#else
using GdiDeviceContext = GdiDeviceContextBasic;
auto& gTheme = gThemeBasic;
#endif

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
        "Status: %s %d%%\r\n\r\n"
        "%s\r\n" // instructions
        ;

    int32_t renderPercent = gpRenderer->gSongRendered.AsPercentOf(gpRenderer->gSongLength);

    // actual precalc time will be the sooner of: [max_precalc_allowed, complete render]
    gPrecalcProgressPercent = std::max(renderPercent, gpRenderer->gRenderTime.AsPercentOf(WSTime::FromMilliseconds(gMaxPrecalcMilliseconds)));

    char saveIndicatorText[1000] = { 0 };
    if (gSavedToFilename[0]) {
        sprintf(saveIndicatorText, "Saved to \"%s\"", gSavedToFilename);
    }
    else if (gFilename[0]) {
        sprintf(saveIndicatorText, "After rendering, will be saved to \"%s\"", gFilename);
    }

    sprintf(gWindowText, format,
        TEXT_INTRO,
        saveIndicatorText,
        (gPrecalcProgressPercent < 100) ? "Precalculating..." : ((renderPercent < 100) ? "Rendering..." : "Done"),
        (gPrecalcProgressPercent < 100) ? gPrecalcProgressPercent : ((renderPercent < 100) ? renderPercent : 0),
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

    int32_t renderPercent = gpRenderer->gSongRendered.AsPercentOf(gpRenderer->gSongLength);

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
        TEXT_INTRO,
        saveIndicatorText,
        renderPercent,
        renderRate / 100, renderRate % 100,
        gpRenderer->gpRenderer->mpGraphRunner->mThreadCount,
        gpRenderer->gRenderTime.GetMinutes(), gpRenderer->gRenderTime.GetSecondsOfMinute(), gpRenderer->gRenderTime.GetTenthsOfSecondsOfSeconds(),
        remainingRenderTime.GetMinutes(), remainingRenderTime.GetSecondsOfMinute(), remainingRenderTime.GetTenthsOfSecondsOfSeconds(),
        estTotalRenderTime.GetMinutes(), estTotalRenderTime.GetSecondsOfMinute(), estTotalRenderTime.GetTenthsOfSecondsOfSeconds(),
        remainingPrecalcTime.GetMinutes(), remainingPrecalcTime.GetSecondsOfMinute(), remainingPrecalcTime.GetTenthsOfSecondsOfSeconds(),
        totalPrecalcTime.GetMinutes(), totalPrecalcTime.GetSecondsOfMinute(), totalPrecalcTime.GetTenthsOfSecondsOfSeconds()
    );


#endif

}

void WriteWave()
{
    OneShotWaveWriter::WriteWAV(gFilename, *gpRenderer);
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

#ifdef FANCY_PAINT
void RenderWaveform_Fancy(GdiDeviceContext& dc)
{
    static constexpr auto waveformRect = gThemeFancy.grcWaveform;
    dc.SolidFill(waveformRect, gTheme.WaveformUnrenderedHatch1);
    static constexpr  auto midY = waveformRect.GetMidY();
    static constexpr  auto left = waveformRect.GetLeft();
    //Point renderCursorP1{ waveformRect.GetLeft() + gpWaveformGen->mProcessedWidth, waveformRect.GetTop() };
    //Point renderCursorP2{ renderCursorP1.GetX(), waveformRect.GetBottom() };
    auto lock = gpRenderer->gCritsec.Enter(); // don't give waveformgen its own critsec for 1) complexity (lock hierarchies!) and 2) code size.
    for (int i = 0; i < gpWaveformGen->mProcessedWidth; ++i) {
        auto h = gpWaveformGen->mHeights[i];

        //const Point p1{ left + i, midY - h };
        //const Point p2{ p1.GetX(), midY + h };
        // distance to render cursor
        //int distToRenderCursor = renderCursorP1.GetX() - p1.GetX();
        //float t = float(distToRenderCursor) / gWaveformGradientMaxDistancePixels;
        //t = WaveSabreCore::M7::math::clamp01(1.0f - t);
        //t = WaveSabreCore::M7::math::modCurve_xN11_kN11(t, -0.95f);
        //COLORREF g = RGB(
        //    WaveSabreCore::M7::math::lerp(GetRValue(gTheme.WaveformForeground), GetRValue(gTheme.RenderCursorColor), t),
        //    WaveSabreCore::M7::math::lerp(GetGValue(gTheme.WaveformForeground), GetGValue(gTheme.RenderCursorColor), t),
        //    WaveSabreCore::M7::math::lerp(GetBValue(gTheme.WaveformForeground), GetBValue(gTheme.RenderCursorColor), t)
        //    );
        //
        //dc.DrawLine(p1, p2, gTheme.WaveformForeground);
        dc.SolidFill({ left + i, midY - h, 1, h * 2 }, gTheme.WaveformForeground);
    }
    //dc.HatchFill(gpWaveformGen->GetUnprocessedRect(), gTheme.WaveformUnrenderedHatch1, gTheme.WaveformUnrenderedHatch2);
    
    //Point midLineP1{ left, midY };
    //Point midLineP2{ renderCursorP1.GetX(), midY };
    //dc.DrawLine(midLineP1, midLineP2, gTheme.WaveformZeroLine);

    //dc.DrawLine(renderCursorP1, renderCursorP2, gTheme.RenderCursorColor);

    //if (gpPlayer->IsPlaying()) {
    //    auto playFrames = gpPlayer->gPlayTime.GetFrames();
    //    //auto c = (playFrames >= gpRenderer->gSongRendered.GetFrames()) ? gTheme.PlayCursorBad : gTheme.PlayCursorGood;
    //    auto c = gTheme.PlayCursor;

    //    int playCursorX = MulDiv(playFrames, gpWaveformGen->mRect.GetWidth(), gpRenderer->gSongLength.GetFrames());
    //    static constexpr int gPlayCursorWidth = 4;
    //    Rect rc{ gpWaveformGen->mRect.GetLeft() + playCursorX - gPlayCursorWidth / 2, renderCursorP1.GetY(), gPlayCursorWidth, gpWaveformGen->mRect.GetHeight() };

    //    dc.SolidFill(rc, c);
    //}
}
void handlePaint_Fancy()
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
    //lf.lfHeight = MulDiv(lf.lfHeight, gTheme.gFontHeightMulPercent, 100); // change font height
    //lf.lfWeight = 1000;
    //HFONT hCustomFont = CreateFontIndirect(&lf); // create a new font based on the modified information

    //HFONT hOldFont = (HFONT)SelectObject(dc.mDC, hCustomFont);
    RenderWaveform_Fancy(dc);

    dc.DrawText_(gWindowText, gTheme.grcText, gTheme.TextColor);

    //if (gPrecalcProgressPercent < 100) {
    //    //dc.SolidFill(gTheme.grcPrecalcProgress, gTheme.PrecalcProgressBackground);
    //    dc.SolidFill(gTheme.grcPrecalcProgress.LeftAlignedShrink(gPrecalcProgressPercent), gTheme.PrecalcProgressForeground);
    //    char sz[100];
    //    sprintf(sz, "Precalculating %d%%...", gPrecalcProgressPercent);
    //    dc.DrawText_(sz, gTheme.grcPrecalcProgress, gTheme.PrecalcTextColor);
    //}

    // present the back buffer
    BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, dc.mDC, 0, 0, SRCCOPY);

    //SelectObject(dc.mDC, hOldFont);
    //DeleteObject(hCustomFont);
    SelectObject(dc.mDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(dc.mDC);

    EndPaint(hMain, &ps);
}
#else 

void handlePaint_Basic()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hMain, &ps);

    GdiDeviceContext dc{ hdc };

    dc.SolidFill(gTheme.grcWindow, gTheme.PrecalcProgressBackground);
    dc.SolidFill(gTheme.grcWindow.LeftAlignedShrink(gPrecalcProgressPercent), gTheme.PrecalcProgressForeground);

    dc.DrawText_(gWindowText, gTheme.grcText, gTheme.TextColor);

    EndPaint(hMain, &ps);
}


#endif




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
#ifndef WS_EXEPLAYER_RELEASE_FEATURES
    case WM_LBUTTONUP:
    {
        Point p{ LOWORD(lParam), HIWORD(lParam) };
        if (!gTheme.grcWaveform.ContainsPoint(p)) {
            return 0;
        }
        int xPos = LOWORD(lParam) - gTheme.grcWaveform.GetLeft();
        uint32_t ms = MulDiv(xPos, gpRenderer->gSongLength.GetMilliseconds(), gTheme.grcWaveform.GetWidth());
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
#ifdef ALLOW_EARLY_PLAY
            gpPlayer->PlayFrom(WSTime::FromFrames(0));
#else
            if (gPrecalcProgressPercent >= 100) {
                gpPlayer->PlayFrom(WSTime::FromFrames(0));
            }
#endif
            return 0;
#ifndef WS_EXEPLAYER_RELEASE_FEATURES
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
#ifdef FANCY_PAINT
        handlePaint_Fancy();
#else
        handlePaint_Basic();
#endif
        return 0;
    }
    case WM_CLOSE:
        ExitProcess(0); // avoid importing postquitmessage and having to release tons of stuff (bits bits bits)
        return 0;
    }

    // since we're completely hijacking the window don't call old proc.
    return ::DefWindowProcA(hwnd, msg, wParam, lParam);
}


#ifdef MIN_SIZE_REL
extern "C" int WinMainCRTStartup()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdline, int show)
#endif
{

#ifdef HIGH_PROCESS_PRIORITY
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

    gWindowText = new char[60000];
    gWindowText[0] = 0;

    hMain = ::CreateWindowExA(0, "EDIT", TEXT_INTRO, WS_VISIBLE | ES_READONLY | ES_MULTILINE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, gTheme.grcWindow.GetWidth(), gTheme.grcWindow.GetHeight(), 0, 0, 0, 0);

    auto oldProc = (WNDPROC)::SetWindowLongPtrA(hMain, GWLP_WNDPROC, (LONG_PTR)WindowProc);

    SongRenderer::Song song;
    song.blob = SongBlob;
    song.factory = SongFactory;
    gpRenderer = new Renderer(hMain, song);
    gpPlayer = new WaveOutPlayer(*gpRenderer);

#ifdef FANCY_PAINT
    gpWaveformGen = new WaveformGen(*gpRenderer);
    gpRenderer->Begin(gpWaveformGen);
#else
    gpRenderer->Begin();
#endif

    SetTimer(hMain, 0, gGeneralSleepPeriodMS, 0);

    MSG msg;
    while (true) {
        GetMessage(&msg, 0, 0, 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
