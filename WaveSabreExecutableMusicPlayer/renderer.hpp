#pragma once

#include <WaveSabreCore.h>
#include <WaveSabrePlayerLib.h>
using namespace WaveSabrePlayerLib;

#include "utils.hpp"
#include "config.hpp"
#include "rendered.hpp"

using RendererOfChoice = WaveSabrePlayerLib::SongRenderer2;

struct Renderer
{
    enum class RenderStatus
    {
        Idle,
        Rendering,
        Done,
    };
    RendererOfChoice::Song gSong;
    WSTime gSongLength;
    WSTime gSongRendered;
    WSTime gRenderTime;

    RendererOfChoice* gpRenderer = nullptr;
    RendererOfChoice::Sample* gpBuffer = nullptr;
    int gAllocatedSampleCount = 0; // stereo samples

    RenderStatus mRenderStatus = RenderStatus::Idle;
    DWORD renderingStartedTick = 0;
    WAVEFORMATEX WaveFMT;
    WAVEHDR WaveHDR;
    WaveSabreCore::CriticalSection gCritsec;
    HWND mhWndNotify;
    HANDLE mhRenderThread;
    //ISampleProcessor* mpAdditionalProcessor = nullptr;
    DWORD mProcessorCount = 0;

    Renderer(HWND hWndNotify) : 
        mhWndNotify(hWndNotify)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        gSong.blob = SongBlob;
        gSong.factory = SongFactory;
        mProcessorCount = sysInfo.dwNumberOfProcessors;
        gpRenderer = new RendererOfChoice(&gSong, mProcessorCount);
        gSampleRate = gpRenderer->GetSampleRate();

        gSongLength.SetMilliseconds(WaveSabreCore::M7::math::DoubleToLong(gpRenderer->GetLength() * 1000));
        static_assert(RendererOfChoice::NumChannels == 2, "everything here assumes stereo");
        gAllocatedSampleCount = gSongLength.GetStereoSamples() + (gSampleRate * 2); // allocate more than the song requires for good measure.
        gpBuffer = new RendererOfChoice::Sample[gAllocatedSampleCount];
        auto bufferSizeBytes = gAllocatedSampleCount * sizeof(RendererOfChoice::Sample);
        memset(gpBuffer, 0, bufferSizeBytes);

        WaveFMT.wFormatTag = WAVE_FORMAT_PCM;
        WaveFMT.nChannels = 2;
        WaveFMT.nSamplesPerSec = gSampleRate;
        WaveFMT.wBitsPerSample = sizeof(RendererOfChoice::Sample) * 8;
        WaveFMT.nBlockAlign = (WaveFMT.nChannels * WaveFMT.wBitsPerSample) / 8;
        WaveFMT.nAvgBytesPerSec = WaveFMT.nSamplesPerSec * WaveFMT.nBlockAlign;
        WaveFMT.cbSize = 0;

        WaveHDR =
        {
          (LPSTR)gpBuffer,
          (DWORD)bufferSizeBytes,
          0,
        };
    }

    //void Begin(ISampleProcessor* pAdditionalProcessor)
    //{
    //    mpAdditionalProcessor = pAdditionalProcessor;
    //    renderingStartedTick = GetTickCount();
    //    mhRenderThread = ::CreateThread(0, 0, RenderThread, this, 0, 0);
    //}

    void Begin()
    {
        renderingStartedTick = GetTickCount();
        mhRenderThread = ::CreateThread(0, 0, RenderThread, this, 0, 0);
    }

    RenderStatus GetRenderStatus() const
    {
        return this->mRenderStatus;
    }

    static DWORD WINAPI RenderThread(LPVOID capture)
    {
        ((Renderer*)capture)->RenderThread2();
        // HACK / TODO: crash when this thread exits. my guess is because i have elided freeing resources to save bits; it's hard to repro and not important so just ... don't return from this thread.
        while (true) {
            Sleep(100);
        }
        return 0;
    }

    DWORD RenderThread2()
    {
        mRenderStatus = RenderStatus::Rendering;
        for (int i = 0; i < gSongLength.GetStereoSamples(); i += gBlockSizeSamples)
        {
            gpRenderer->RenderSamples(gpBuffer + i, gBlockSizeSamples);
            gSongRendered.SetStereoSamples(i);
            gRenderTime.SetMilliseconds(GetTickCount() - renderingStartedTick);

            //mpAdditionalProcessor->ProcessSamples(gpBuffer + i, gBlockSizeSamples);
        }

        mRenderStatus = RenderStatus::Done;
        SendMessageA(this->mhWndNotify, WM_RENDERINGCOMPLETE, 0, 0);
        return 0;
    }
};



