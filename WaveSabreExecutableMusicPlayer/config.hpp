#pragma once

#include "utils.hpp"

//static constexpr uint32_t gSongPaddingMS = 3000; // additional padding to add at the end of the track to account for fadeouts whatever
static constexpr uint32_t gBlockSizeSamples = 256; // don't try to bite off too much; modulations will be too loose. try to replicate DAW behavior. Note these are samples not frames. 128 sample buffer probably means 256 here.
#define TEXT_INTRO "(song title here)\r\n~~ by tenfour/RBBS for Revision 2023\r\n"

struct
{
    COLORREF TextColor = RGB(255, 255, 0);
    COLORREF TextShadowColor = RGB(0, 0, 0);
    COLORREF WindowBackground = RGB(0, 0, 0);
    COLORREF WaveformForeground = RGB(0, 64, 64);
    COLORREF WaveformBackground = RGB(0, 24, 24);
    COLORREF WaveformZeroLine = RGB(64, 64, 64);
    COLORREF WaveformUnrenderedHatch1 = RGB(60, 60, 60);
    COLORREF WaveformUnrenderedHatch2 = RGB(30, 30, 30);
    COLORREF RenderCursorColor = RGB(212, 0, 212);
    COLORREF PlayCursorGood = RGB(0, 192, 0);
    COLORREF PlayCursorBad = RGB(255, 0, 0);
} gColorScheme;

static constexpr Rect grcWindow{ 0, 0, 1400, 600 };
static constexpr Rect grcText{ grcWindow };// { 0, 0, 400, 400 };
static constexpr Rect grcWaveform{ grcWindow };// { 0, 0, 1400, 600 };
static constexpr uint32_t gGeneralSleepPeriodMS = 30;

static constexpr int gWaveformGradientMaxDistancePixels = 500;
