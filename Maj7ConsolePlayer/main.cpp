#include <WaveSabrePlayerLib/WavWriter.h>

#include "../WaveSabreExecutableMusicPlayer/rendered.hpp"

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    return 1;
  }

  const char* outputPath = argv[1];
  WaveSabrePlayerLib::WavWriter writer(&Song, 1);
  writer.Write(outputPath, nullptr, nullptr);
  return 0;
}
