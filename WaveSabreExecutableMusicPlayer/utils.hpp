#pragma once

#include <Windows.h>
#include <string.h>
#include <typeinfo>

// set this in order to use WSTime
static DWORD gSampleRate = 0;

//#define WM_WRITEWAV (WM_APP + 1)
#define WM_RENDERINGCOMPLETE (WM_APP + 2)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Point
{
    const int mX = 0;
    const int mY = 0;
public:
    constexpr Point() = default;
    constexpr Point(int x, int y) : mX(x), mY(y) {}
    constexpr int GetX() const { return mX; }
    constexpr int GetY() const { return mY; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Rect
{
    Point mOrig;
    const int mWidth = 0;
    const int mHeight = 0;
public:
    constexpr Rect() = default;
    constexpr Rect(int x, int y, int width, int height) : mOrig(x, y), mWidth(width), mHeight(height) {}
    //constexpr Rect(const Point& orig, int width, int height) : mOrig(orig), mWidth(width), mHeight(height) {}
    constexpr int GetLeft() const { return mOrig.GetX(); }
    constexpr int GetTop() const { return mOrig.GetY(); }
    constexpr int GetWidth() const { return mWidth; }
    constexpr int GetHeight() const { return mHeight; }
    constexpr int GetMidY() const { return mOrig.GetY() + mHeight / 2; }
    constexpr int GetRight() const { return mOrig.GetX() + mWidth; }
    constexpr int GetBottom() const { return mOrig.GetY() + mHeight; }
    constexpr bool ContainsPoint(const Point& pt) const
    {
        if (pt.GetX() < mOrig.GetX()) return false;
        if (pt.GetY() < mOrig.GetY()) return false;
        if (pt.GetX() >= this->GetRight()) return false;
        if (pt.GetY() >= this->GetBottom()) return false;
        return true;
    }
    Rect LeftAlignedShrink(int percent) const
    {
        return Rect{ mOrig.GetX(), mOrig.GetY(), mWidth * percent / 100, mHeight };
    }
    RECT GetRECT(/*huhu*/) const {
        return { mOrig.GetX(), mOrig.GetY(), GetRight(), GetBottom() };
    }
    constexpr Rect Offset(int x, int y) const {
        return Rect{ mOrig.GetX() + x, mOrig.GetY() + y, mWidth, mHeight };
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WSTime
{
    int32_t mFrames = 0; // stereo samples
    explicit WSTime(int32_t f) : mFrames(f) {}
public:
    WSTime() {}
    static int32_t MillisecondsToFrames(int32_t ms)
    {
        return MulDiv(ms, gSampleRate, 1000);
        // this will overflow for even moderate ms amounts. need to be careful. for example a 10 minute value
        // = 60 sec = 60000 ms * 44100 = 2,646,000,000, overflowing. so assume the samplerate is a multiple of 10.
        // 2,147,483,647
        // that gives us max ms of 486,957, or ~30 minutes.
        //auto srDiv10 = gSampleRate / 10;
        //return ms * srDiv10 / 100;
    }
    int32_t GetFrames() const {
        return (int32_t)mFrames;
    }
    int32_t GetStereoSamples() const {
        return int32_t(mFrames * 2);
    }
    int32_t GetMilliseconds() const {
        return MulDiv(mFrames, 1000, gSampleRate);
    }
    int32_t GetBytes() const {
        static_assert(std::is_same_v<SongRenderer::Sample, int16_t>, "assuming 16-bit sample format");
        return GetStereoSamples() * sizeof(int16_t);
    }
    int GetMinutes() const {
        return GetMilliseconds() / 60000;
    }
    int GetSecondsOfMinute() const {
        return (GetMilliseconds() % 60000) / 1000;
    }
    int GetMillisecondsOfSeconds() const {
        return (GetMilliseconds() % 1000);
    }
    int GetTenthsOfSecondsOfSeconds() const {
        return GetMillisecondsOfSeconds() / 100;
    }
    void SetMilliseconds(int32_t ms) {
        mFrames = MillisecondsToFrames(ms);
    }
    void SetStereoSamples(int32_t ss) {
        mFrames = ss / 2;
    }
    void SetFrames(int32_t f) {
        mFrames = f;
    }
    static WSTime FromMilliseconds(int32_t ms)
    {
        return WSTime{ MillisecondsToFrames(ms) };
    }
    static WSTime FromFrames(int32_t f)
    {
        return WSTime{ f };
    }

    // returns 0-100, where rhs is the "total" and this is x.
    int32_t AsPercentOf(const WSTime& rhs) const
    {
        if (mFrames <= 0) return 0;
        if (mFrames >= rhs.mFrames) return 100;
        return MulDiv(mFrames, 100, rhs.mFrames);
        //return int32_t(mFrames * 100) / int32_t(rhs.mFrames); // avoid __alldiv
    }

    // returns this value as a rate of another value, multiplied by 100. so if this is 6, and rhs is 10, returns 60.
    // if rhs is 0, returns 100.
    // similar to AsPercentOf, but for a different purpose.
    int32_t AsRateOf_x100(const WSTime& rhs) const
    {
        if (mFrames <= 0) return 0;
        if (rhs.mFrames <= 0) return 100;
        return MulDiv(mFrames, 100, rhs.mFrames);
        //return int32_t(mFrames * 100) / int32_t(rhs.mFrames); // avoid __alldiv
    }
    WSTime operator -(const WSTime& rhs) const
    {
        return WSTime{ mFrames - rhs.mFrames };
    }
    WSTime operator +(const WSTime& rhs) const
    {
        return WSTime{ mFrames + rhs.mFrames };
    }
    bool operator >= (const WSTime& rhs) const {
        return mFrames >= rhs.mFrames;
    }
    WSTime MultipliedByRate_x100(int32_t r100) const
    {
        auto frames = MulDiv(mFrames, r100, 100);// int32_t(mFrames * r100) / 100;
        return WSTime{ frames };
    }
    WSTime DividedByRate_x100(int32_t r100) const
    {
        if (r100 == 0) {
            return WSTime{ mFrames };
        }
        auto frames = MulDiv(mFrames, 100, r100);// int32_t(mFrames * r100) / 100;
        //auto frames = int32_t(mFrames * 100) / r100; // avoid __alldiv
        return WSTime{ frames };
    }

    void Max0() { // just set a floor of 0.
        if (mFrames < 0) mFrames = 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GdiDeviceContext
{
    HDC mDC;
    explicit GdiDeviceContext(HDC dc) : mDC(dc)
    {
    }
    void DrawText_(const char* sz, const Rect& bounds, COLORREF fore, COLORREF shadow) const
    {
        SetBkMode(mDC, TRANSPARENT);
        SetTextColor(mDC, shadow);
        auto rcText = bounds.GetRECT();
        auto rcShadow = rcText;
        rcShadow.left--;
        rcShadow.top--;
        ::DrawTextA(mDC, sz, -1, &rcShadow, 0);
        rcShadow.left += 2;
        rcShadow.top += 2;
        ::DrawTextA(mDC, sz, -1, &rcShadow, 0);

        SetTextColor(mDC, fore);
        ::DrawTextA(mDC, sz, -1, &rcText, 0);
        //DrawText_(gWindowText, grcText.Offset(-1, -1));
        //dc.DrawText_(gWindowText, grcText.Offset(1, 1));
        //dc.DrawText_(gWindowText, grcText);



        //::DrawTextA(mDC, sz, -1, &rcText, 0);
    }
    void SetForeBackColor(COLORREF foreColor, COLORREF backColor) const
    {
        SetTextColor(mDC, foreColor);
        SetBkColor(mDC, backColor);
    }
    void HatchFill(const Rect& bounds, COLORREF foreColor, COLORREF backColor) const
    {
        SetBkColor(mDC, backColor);
        //SetBkMode(mDC, OPAQUE);
        HBRUSH hBrush = CreateHatchBrush(HS_BDIAGONAL, foreColor);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(mDC, hBrush);
        RECT rc = bounds.GetRECT();
        FillRect(mDC, &rc, hBrush);
        SelectObject(mDC, hOldBrush);
        DeleteObject(hBrush);
    }
    void DrawLine(const Point& p1, const Point& p2, COLORREF color) const
    {
        // Draw a red line from (10, 10) to (100, 100)
        HPEN hPen = CreatePen(PS_SOLID, 1, color);
        HPEN hOldPen = (HPEN)SelectObject(mDC, hPen);
        MoveToEx(mDC, p1.GetX(), p1.GetY(), NULL);
        LineTo(mDC, p2.GetX(), p2.GetY());
        SelectObject(mDC, hOldPen);
        DeleteObject(hPen);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ISampleProcessor
{
    virtual void ProcessSamples(int16_t* samples, int nSamples) = 0;
};

