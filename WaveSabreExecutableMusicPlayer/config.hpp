#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TEXT_INTRO "Bright Velvet\r\nby tenfour/RBBS for Revision 2023\r\n\r\n"

#define WS_EXEPLAYER_RELEASE_FEATURES

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"

struct
{
#ifdef FANCY_PAINT
    COLORREF TextColor = RGB(255, 255, 0);
    COLORREF TextShadowColor = RGB(0, 0, 0);
    COLORREF WindowBackground = RGB(0, 0, 0);
    COLORREF WaveformBackground = RGB(12, 16, 16);
    COLORREF WaveformForeground = RGB(16, 48, 48);
    COLORREF WaveformZeroLine = RGB(48, 48, 48);
    COLORREF WaveformUnrenderedHatch1 = RGB(60, 60, 60);
    COLORREF WaveformUnrenderedHatch2 = RGB(30, 30, 30);
    COLORREF RenderCursorColor = RGB(80, 0, 80);
    COLORREF PlayCursorGood = RGB(0, 192, 0);
    COLORREF PlayCursorBad = RGB(255, 0, 0);

    COLORREF PrecalcProgressBackground = RGB(0, 24, 24);
    COLORREF PrecalcProgressForeground = RGB(0, 96, 96);
    COLORREF PrecalcTextColor = RGB(0, 192, 192);
    COLORREF PrecalcTextShadowColor = PrecalcProgressForeground;
#else
    COLORREF TextColor = RGB(255, 255, 0);
    COLORREF PrecalcProgressBackground = RGB(0, 24, 24);
    COLORREF PrecalcProgressForeground = RGB(0, 96, 96);
#endif
} gColorScheme;

#ifdef FANCY_PAINT
static constexpr Rect grcWindow{ 0, 0, 1100, 500 };
static constexpr int gFontHeightMulPercent = 125;
#else
static constexpr Rect grcWindow{ 0, 0, 700, 500 };
#endif
static constexpr Rect grcText{ grcWindow.Offset(20, 10) };// { 0, 0, 400, 400 };
static constexpr Rect grcWaveform{ grcWindow };// { 0, 0, 1400, 600 };
static constexpr uint32_t gGeneralSleepPeriodMS = 30;

static constexpr Rect grcPrecalcProgress{ 0, grcWindow.GetHeight() - 100, grcWindow.GetWidth(), 100};

static constexpr int gWaveformGradientMaxDistancePixels = 500;

// in an ideal world we could know when it's 100% safe to play the track without "buffering".
// the track renders at different rates at different places so it's not possible to really know reliably.
// therefore, only play the track when either 1) the track is finished rendering, or 2) the max precalc time allowed has passed.
// so what if the max precalc time is not enough? in this case simply present the user with the option to play.
// that will buy us at least few more seconds.
static constexpr int32_t gMaxPrecalcMilliseconds = 30000;

static constexpr uint32_t gBlockSizeSamples = 256; // don't try to bite off too much; modulations will be too loose. try to replicate DAW behavior. Note these are samples not frames. 128 sample buffer probably means 256 here.

