#include <WaveSabrePlayerLib/WavWriter.h>

#include "../WaveSabreExecutableMusicPlayer/rendered.hpp"

void ProgressCallback(double progress, void *data)
{
  printf("%.0f%%\n", progress * 100.0);
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    printf("Usage: %s path\\to\\file.wav\n", argv[0]);
  }
  else
  {
    const char* outputPath = argv[1];
    WaveSabrePlayerLib::WavWriter writer(&Song, 16);
    writer.Write(outputPath, ProgressCallback, nullptr);
  }
  ExitProcess(0); // song renderer spawns a bunch of threads; this shuts down without having to coordinate threads.
}
