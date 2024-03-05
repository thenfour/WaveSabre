
// Basically copied from exe music player,
// but the standalone player:
// - doesn't require size optimizations. however MinSizeRel still elides modern CRT so avoid exceptions, std:: objects etc.
// - is intended mostly for invocation from ProjectManager. Therefore let's not hold back on diagnostic info.

#include <Windows.h>
#include <string.h>
#include <Commctrl.h>
#include <Commdlg.h>
#pragma comment(lib, "Comdlg32.lib")

//#include <vector>
#include <string>

#include <WaveSabreCore.h>
#include <WaveSabrePlayerLib.h>
using namespace WaveSabrePlayerLib;
using namespace WSPlayerApp;

char gFilename[MAX_PATH] = { 0 }; // set this to save upon completion
char gSavedToFilename[MAX_PATH] = { 0 }; // gets populated upon successful save.

auto& gTheme = gThemeFancy;

WaveSabreCore::Device *SongFactory(SongRenderer::DeviceId id)
{
	switch (id)
	{
	case SongRenderer::DeviceId::Leveller: return new WaveSabreCore::Leveller();
	case SongRenderer::DeviceId::Crusher: return new WaveSabreCore::Crusher();
	case SongRenderer::DeviceId::Echo: return new WaveSabreCore::Echo();
	case SongRenderer::DeviceId::Chamber: return new WaveSabreCore::Chamber();
	case SongRenderer::DeviceId::Twister: return new WaveSabreCore::Twister();
	case SongRenderer::DeviceId::Cathedral: return new WaveSabreCore::Cathedral();
	case SongRenderer::DeviceId::Maj7: return new WaveSabreCore::M7::Maj7();
    case SongRenderer::DeviceId::Maj7Width: return new WaveSabreCore::Maj7Width();
    case SongRenderer::DeviceId::Maj7Comp: return new WaveSabreCore::Maj7Comp();
    case SongRenderer::DeviceId::Maj7Sat: return new WaveSabreCore::Maj7Sat();
    }
	return nullptr;
}


const char * gStatus = "";
char* gWindowText = nullptr;
HWND hMain;

WSPlayerApp::Renderer* gpRenderer = nullptr;
WSPlayerApp::WaveOutPlayer* gpPlayer = nullptr;

WaveformGen* gpWaveformGen = nullptr;

// position between min(precalctime_max, render_progress)
int32_t gPrecalcProgressPercent = 0;

void UpdateStatusText()
{
    if (gpRenderer)
    {
        auto lock = gpRenderer->gCritsec.Enter();

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
            gStatus,
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
    }
    else {
        sprintf(gWindowText, "%s", gStatus);
    }
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


void RenderWaveform_Fancy(GdiDeviceContextFancy& dc)
{
    if (!gpWaveformGen) return;
    auto lock = gpRenderer->gCritsec.Enter(); // don't give waveformgen its own critsec for 1) complexity (lock hierarchies!) and 2) code size.

    dc.HatchFill(gpWaveformGen->mRect, gTheme.WaveformBackground, gTheme.WaveformBackground);
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
        COLORREF g = RGB(
            WaveSabreCore::M7::math::lerp(GetRValue(gTheme.WaveformForeground), GetRValue(gTheme.RenderCursorColor), t),
            WaveSabreCore::M7::math::lerp(GetGValue(gTheme.WaveformForeground), GetGValue(gTheme.RenderCursorColor), t),
            WaveSabreCore::M7::math::lerp(GetBValue(gTheme.WaveformForeground), GetBValue(gTheme.RenderCursorColor), t)
        );

        dc.DrawLine(p1, p2, g);
    }
    dc.HatchFill(gpWaveformGen->GetUnprocessedRect(), gTheme.WaveformUnrenderedHatch1, gTheme.WaveformUnrenderedHatch2);

    Point midLineP1{ left, midY };
    Point midLineP2{ renderCursorP1.GetX(), midY };
    dc.DrawLine(midLineP1, midLineP2, gTheme.WaveformZeroLine);

    dc.DrawLine(renderCursorP1, renderCursorP2, gTheme.RenderCursorColor);

    if (gpPlayer->IsPlaying()) {
        auto playFrames = gpPlayer->gPlayTime.GetFrames();
        auto c = (playFrames >= gpRenderer->gSongRendered.GetFrames()) ? gTheme.PlayCursorBad : gTheme.PlayCursorGood;

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
    GdiDeviceContextFancy dc{ CreateCompatibleDC(hdc) };
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(dc.mDC, hBitmap);

    HFONT hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    LOGFONT lf;
    GetObject(hFont, sizeof(LOGFONT), &lf);
    lf.lfHeight = MulDiv(lf.lfHeight, gTheme.gFontHeightMulPercent, 100); // change font height
    lf.lfWeight = 1000;
    HFONT hCustomFont = CreateFontIndirect(&lf); // create a new font based on the modified information

    HFONT hOldFont = (HFONT)SelectObject(dc.mDC, hCustomFont);
    RenderWaveform_Fancy(dc);

    dc.DrawText_(gWindowText, gTheme.grcText, gTheme.TextColor, gTheme.TextShadowColor);

    if (gPrecalcProgressPercent < 100) {
        dc.HatchFill(gTheme.grcPrecalcProgress, gTheme.PrecalcProgressBackground, gTheme.PrecalcProgressBackground);
        dc.HatchFill(gTheme.grcPrecalcProgress.LeftAlignedShrink(gPrecalcProgressPercent), gTheme.PrecalcProgressForeground, gTheme.PrecalcProgressForeground);
        char sz[100];
        sprintf(sz, "Precalculating %d%%...", gPrecalcProgressPercent);
        dc.DrawText_(sz, gTheme.grcPrecalcProgress, gTheme.PrecalcTextColor, gTheme.PrecalcTextShadowColor);
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
    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_F7:
            handleSave();
            return 0;
        case VK_F5:
            gpPlayer->PlayFrom(WSTime::FromFrames(0));
            return 0;
        case VK_F6:
            gpPlayer->Reset();
            return 0;
        case VK_F8:
            //WaveSabrePlayerLib::gpGraphProfiler->Dump();
            return 0;
        }

        break;
    }
    case WM_PAINT:
    {
        handlePaint_Fancy();
        return 0;
    }
    case WM_CLOSE:
        ExitProcess(0); // avoid importing postquitmessage and having to release tons of stuff (bits bits bits)
        return 0;
    }

    // since we're completely hijacking the window don't call old proc.
    return ::DefWindowProcA(hwnd, msg, wParam, lParam);
}

inline uint8_t* ReadBinaryFile(const char* filename)
{
    HANDLE file = ::CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        // handle error
        return nullptr;
    }

    DWORD file_size = ::GetFileSize(file, NULL);
    if (file_size == INVALID_FILE_SIZE)
    {
        // handle error
        ::CloseHandle(file);
        return nullptr;
    }

    uint8_t* buffer = new byte[file_size];
    DWORD bytes_read;
    if (!::ReadFile(file, buffer, file_size, &bytes_read, NULL) || bytes_read != file_size)
    {
        // handle error
        ::CloseHandle(file);
        return nullptr;
    }

    ::CloseHandle(file);
    return buffer;
}


int main(int argc, char **argv)
{
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    gWindowText = new char[60000];
    gWindowText[0] = 0;

    hMain = ::CreateWindowExA(0, "EDIT", "WaveSabre standalone player", WS_VISIBLE | ES_READONLY | ES_MULTILINE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, gTheme.grcWindow.GetWidth(), gTheme.grcWindow.GetHeight(), 0, 0, 0, 0);

    auto oldProc = (WNDPROC)::SetWindowLongPtrA(hMain, GWLP_WNDPROC, (LONG_PTR)WindowProc);

    const char * binPath;
    if (argc > 1) {
        binPath = argv[1];
    }
    if (argc > 3 && !strcmp(argv[2], "-w")) {
        strcpy(gFilename, argv[3]);
    }

    auto blob = ReadBinaryFile(binPath);
    if (!blob) {
        gStatus = "Error reading file";
    }
    if (blob) {
        SongRenderer::Song song;
        song.blob = blob;
        song.factory = SongFactory;
        gpRenderer = new Renderer(hMain, song);
        gpPlayer = new WaveOutPlayer(*gpRenderer);

        gpWaveformGen = new WaveformGen(gTheme.grcWaveform, *gpRenderer);
        gpRenderer->Begin(gpWaveformGen);
    }
    SetTimer(hMain, 0, gGeneralSleepPeriodMS, 0);

    MSG msg;
    while (true) {
        GetMessage(&msg, 0, 0, 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
