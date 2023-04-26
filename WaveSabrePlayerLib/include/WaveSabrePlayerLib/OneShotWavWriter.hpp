#pragma once

// Writes a WAV file but in one shot rather than incrementally with progress.

#include "PlayerAppUtils.hpp"
#include "PlayerAppRenderer.hpp"


namespace WSPlayerApp
{

    struct OneShotWaveWriter
    {
        static void WriteWAV(const char* path, Renderer& renderer)
        {
            HANDLE ghFile;

            ghFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, 0, 0);
            if (!ghFile || ghFile == INVALID_HANDLE_VALUE)
                return;

            auto write = [&](void* p, int n) {
                DWORD br;
                WriteFile(ghFile, p, n, &br, 0);
            };
            auto writeInt = [&](int p, int n) {
                write(&p, n);
            };

            int bufferSizeBytes = renderer.gSongLength.GetStereoSamples() * sizeof(renderer.gpBuffer[0]);

            // TODO: add metadata via "LIST" section.

            write("RIFF", 4);
            writeInt(bufferSizeBytes + 36, 4);
            write("WAVEfmt ", 8);
            writeInt(16, 4);// format subchunk size
            write(&renderer.WaveFMT, 16);
            write("data", 4);
            // data size 4 bytes
            writeInt(bufferSizeBytes, 4);
            write(renderer.gpBuffer, bufferSizeBytes);
            ::CloseHandle(ghFile);
            ghFile = 0;
        }
    };

} // namespace WSPlayerApp


