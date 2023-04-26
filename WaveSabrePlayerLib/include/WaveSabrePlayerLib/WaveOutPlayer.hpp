#pragma once

#include "PlayerAppUtils.hpp"
#include "PlayerAppRenderer.hpp"
#include <atomic>

namespace WSPlayerApp
{

    struct WaveOutPlayer
    {
        Renderer& mRenderer;

        WSTime gPlayTime; // in song time, not relative to playtimeoffset.
        WSTime mPlayTimeOffset; // specifies where to begin playing from.

        std::atomic_bool mShouldStop = false;
        HANDLE hPlayThread = 0;

        WaveOutPlayer(Renderer& r) : mRenderer(r)
        {
        }

        bool IsPlaying() const {
            return !!hPlayThread;
        }

        void PlayFrom(WSTime t)
        {
            if (t >= mRenderer.gSongLength) return;
            Reset();
            mShouldStop = false;
            mPlayTimeOffset = t;
            hPlayThread = CreateThread(0, 0, PlayThread, this, 0, 0);
        }

        void Reset()
        {
            if (hPlayThread) {
                mShouldStop = true;
                WaitForSingleObject(hPlayThread, INFINITE);
                CloseHandle(hPlayThread);
                hPlayThread = 0;
            }
        }

        static DWORD WINAPI PlayThread(void* pthis) {
            return ((WaveOutPlayer*)pthis)->PlayThread2();
        }
        DWORD PlayThread2()
        {
            HWAVEOUT hWaveOut = 0;
            auto waveHdr = mRenderer.WaveHDR;
            auto byteOffset = mPlayTimeOffset.GetBytes();
            waveHdr.lpData += byteOffset;
            waveHdr.dwBufferLength -= byteOffset;
            MMRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &mRenderer.WaveFMT, NULL, 0, CALLBACK_NULL);
#ifndef MIN_SIZE_REL
            if (result != MMSYSERR_NOERROR)
            {
                return 1;
            }
#endif
            result = waveOutPrepareHeader(hWaveOut, &waveHdr, sizeof(waveHdr));
#ifndef MIN_SIZE_REL
            if (result != MMSYSERR_NOERROR)
            {
                waveOutClose(hWaveOut);
                return 1;
            }
#endif

            result = waveOutWrite(hWaveOut, &waveHdr, sizeof(waveHdr));
#ifndef MIN_SIZE_REL
            if (result == MMSYSERR_NOERROR)
            {
#endif
                while (!mShouldStop)
                {
                    MMTIME MMTime =
                    {
                      TIME_SAMPLES, // bytes is from the file, MS is not always supported...
                      0
                    };
                    waveOutGetPosition(hWaveOut, &MMTime, sizeof(MMTime));
                    gPlayTime.SetFrames(MMTime.u.sample + mPlayTimeOffset.GetFrames());
                    int percentComplete = gPlayTime.AsPercentOf(mRenderer.gSongLength);
                    if (percentComplete >= 100) {
                        break;
                    }
                    Sleep(gGeneralSleepPeriodMS);
                }

                waveOutReset(hWaveOut);
#ifndef MIN_SIZE_REL
            }
            waveOutUnprepareHeader(hWaveOut, &waveHdr, sizeof(waveHdr));
            waveOutClose(hWaveOut);
            hWaveOut = 0;
            gPlayTime.SetFrames(0);
#endif
            return 0;
        }

    };


} // namespace WSPlayerApp

