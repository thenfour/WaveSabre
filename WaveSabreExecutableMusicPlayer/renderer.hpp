#pragma once

#include <WaveSabreCore.h>
#include <WaveSabrePlayerLib.h>
using namespace WaveSabrePlayerLib;

#include "utils.hpp"
#include "config.hpp"
#include "rendered.hpp"

struct Renderer
{
    enum class RenderStatus
    {
        Idle,
        Rendering,
        Done,
    };
    SongRenderer::Song gSong;
    WSTime gSongLength;
    WSTime gSongRendered;
    WSTime gRenderTime;

    SongRenderer* gpRenderer = nullptr;
    SongRenderer::Sample* gpBuffer = nullptr;
    int gAllocatedSampleCount = 0; // stereo samples

    RenderStatus mRenderStatus = RenderStatus::Idle;
    DWORD renderingStartedTick = 0;
    WAVEFORMATEX WaveFMT;
    WAVEHDR WaveHDR;
    WaveSabrePlayerLib::CriticalSection gCritsec;
    HWND mhWndNotify;
    HANDLE mhRenderThread;
    ISampleProcessor* mpAdditionalProcessor = nullptr;
    DWORD mProcessorCount = 0;

    Renderer(HWND hWndNotify) : 
        mhWndNotify(hWndNotify)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        gSong.blob = SongBlob;
        gSong.factory = SongFactory;
        mProcessorCount = sysInfo.dwNumberOfProcessors;
        gpRenderer = new SongRenderer(&gSong, mProcessorCount);
        gSampleRate = gpRenderer->GetSampleRate();

        gSongLength.SetMilliseconds(WaveSabreCore::M7::math::DoubleToLong(gpRenderer->GetLength() * 1000));
        static_assert(SongRenderer::NumChannels == 2, "everything here assumes stereo");
        gAllocatedSampleCount = gSongLength.GetStereoSamples() + (gSampleRate * 2); // allocate more than the song requires for good measure.
        gpBuffer = new SongRenderer::Sample[gAllocatedSampleCount];
        auto bufferSizeBytes = gAllocatedSampleCount * sizeof(SongRenderer::Sample);
        memset(gpBuffer, 0, bufferSizeBytes);

        WaveFMT.wFormatTag = WAVE_FORMAT_PCM;
        WaveFMT.nChannels = 2;
        WaveFMT.nSamplesPerSec = gSampleRate;
        WaveFMT.wBitsPerSample = sizeof(SongRenderer::Sample) * 8;
        WaveFMT.nBlockAlign = (WaveFMT.nChannels * WaveFMT.wBitsPerSample) / 8;
        WaveFMT.nAvgBytesPerSec = WaveFMT.nSamplesPerSec * WaveFMT.nBlockAlign;
        WaveFMT.cbSize = 0;

        WaveHDR =
        {
          (LPSTR)gpBuffer,
          bufferSizeBytes,
          0,
        };
    }

    void Begin(ISampleProcessor* pAdditionalProcessor)
    {
        mpAdditionalProcessor = pAdditionalProcessor;
        renderingStartedTick = GetTickCount();
        mhRenderThread = ::CreateThread(0, 0, RenderThread, this, 0, 0);
    }

    RenderStatus GetRenderStatus() const
    {
        return this->mRenderStatus;
    }

    static DWORD WINAPI RenderThread(LPVOID capture)
    {
        return ((Renderer*)capture)->RenderThread2();
    }

    DWORD RenderThread2()
    {
        mRenderStatus = RenderStatus::Rendering;
        for (int i = 0; i < gSongLength.GetStereoSamples(); i += gBlockSizeSamples)
        {
            gpRenderer->RenderSamples(gpBuffer + i, gBlockSizeSamples);
            gSongRendered.SetStereoSamples(i);
            gRenderTime.SetMilliseconds(GetTickCount() - renderingStartedTick);

            mpAdditionalProcessor->ProcessSamples(gpBuffer + i, gBlockSizeSamples);
        }

        mRenderStatus = RenderStatus::Done;
        SendMessageA(this->mhWndNotify, WM_RENDERINGCOMPLETE, 0, 0);
        return 0;
    }
};



