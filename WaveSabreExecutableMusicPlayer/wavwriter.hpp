#pragma once

#include "utils.hpp"
#include "renderer.hpp"

struct WaveWriter
{
    static HANDLE ghFile;
    static void write(void* p, int n) {
        DWORD br;
        WriteFile(ghFile, p, n, &br, 0);
    }
    static void write(int p, int n) {
        write(&p, n);
    }

    static void WriteWAV(const char *path, Renderer& renderer)
    {
        ghFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, 0, 0);
        if (!ghFile || ghFile == INVALID_HANDLE_VALUE)
            return;

        int bufferSizeBytes = renderer.gSongLength.GetStereoSamples() * sizeof(renderer.gpBuffer[0]);

        // TODO: add metadata via "LIST" section.

        write("RIFF", 4);
        write(bufferSizeBytes + 36, 4);
        write("WAVEfmt ", 8);
        write(16, 4);// format subchunk size
        write(&renderer.WaveFMT, 16);
        write("data", 4);
        // data size 4 bytes
        write(bufferSizeBytes, 4);
        write(renderer.gpBuffer, bufferSizeBytes);
        ::CloseHandle(ghFile);
        ghFile = 0;
    }
};

HANDLE WaveWriter::ghFile = 0;

