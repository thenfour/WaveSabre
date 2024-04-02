#pragma once

#include "PlayerAppUtils.hpp"
#include "PlayerAppRenderer.hpp"
#include "PlayerAppConfig.hpp"


namespace WSPlayerApp
{

    // accepts incoming samples and generates a waveform of a fixed size.
    struct WaveformGen : ISampleProcessor
    {
        //using Heights = WaveSabreCore::M7::Pair<int, int>;
        using Heights = int;
        static_assert(std::is_pod_v<Heights>, "heights is memset so...");

        Renderer& mRenderer;
        //const Rect mRect;

        static constexpr auto mRect = gThemeFancy.grcWaveform;

        const WSTime mSongLength;
        int mProcessedWidth = 0;
        int mUnprocessedWidth = mRect.GetWidth();
        Heights* const mHeights; // for each X pixel, specifies the height of the waveform (L/R pair)
        const int mFramesPerXPixel = 0;
        int mFramesThisPixel = 0; // how many samples has been processed this pixel?

        WaveformGen(Renderer& renderer) :
            //mRect(rect),
            mRenderer(renderer),
            mSongLength(renderer.gSongLength),
            mHeights(new Heights[mRect.GetWidth()]),// don't bother freeing.
            mFramesPerXPixel(std::max(1, mSongLength.GetFrames() / mRect.GetWidth()))
        {
            auto w = mRect.GetWidth();
            memset(mHeights, 0, sizeof(*mHeights) * mRect.GetWidth());
        }

        Rect GetUnprocessedRect() const {
            return Rect{ mRect.GetLeft() + mProcessedWidth, mRect.GetTop(), mUnprocessedWidth, mRect.GetHeight() };
        }

        int SampleToHeight(SongRenderer::Sample sample) const
        {
            static_assert(std::is_integral_v<SongRenderer::Sample>, "expects a fixed-point style sample format");
            //return abs(sample) * (mRect.GetHeight() / 2) / 32768; // 0-32768// the target height is the rect height / 2, because of bipolar
            return abs(sample) * mRect.GetHeight() / 92000; // divide by more to shrink it a bit; it's more comfortable to see the edges.
        }

        void ProcessFrame(int16_t s0, int16_t s1)
        {
            if (mProcessedWidth >= mSongLength.GetFrames()) // just discard excess?
                return;
            auto height1 = SampleToHeight(s0);
            //auto height2 = SampleToHeight(s1);
            auto& slot = mHeights[mProcessedWidth];
            //slot = height1;
            slot = std::max(slot, height1);
            //slot.first = std::max(slot.first, height1);
            //slot.second = std::max(slot.second, height2);

            mFramesThisPixel++;
            if (mFramesThisPixel >= mFramesPerXPixel)
            {
                // advance to next X pixel.
                ++mProcessedWidth;
                --mUnprocessedWidth;
                mFramesThisPixel = 0;
            }
        }

        virtual void ProcessSamples(SongRenderer::Sample* samples, int nSamples) override
        {
            auto lock = mRenderer.gCritsec.Enter();
            for (int i = 0; i < nSamples; i += 2)
            {
                ProcessFrame(samples[i], samples[i + 1]);
            }
        }
    };

} // namespace WSPlayerApp

