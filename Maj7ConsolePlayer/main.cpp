#include <WaveSabrePlayerLib/WavWriter.h>

#include "../WaveSabreExecutableMusicPlayer/rendered.hpp"

void ProgressCallback(double progress, void *data)
{
  printf("Progress: %f%%\n", progress * 100.0);
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    printf("Usage: %s c:\\path\\to\\file.wav\n", argv[0]);
    return 1;
  }

  const char* outputPath = argv[1];
  WaveSabrePlayerLib::WavWriter writer(&Song, 16);
  writer.Write(outputPath, nullptr, nullptr);
  return 0;
}
