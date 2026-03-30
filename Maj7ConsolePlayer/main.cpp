#include <WaveSabrePlayerLib/WavWriter.h>

#include "../WaveSabreExecutableMusicPlayer/rendered.hpp"

//static constexpr int kNumRenderThreads = 48;

//#pragma function(fflush)

// // minimal progress callback...
// void ProgressCallback(double progress, void *data)
// {
//   printf("\r%-10.2f%%", progress * 100.0);
//   //fflush(stdout);
// }

// // with time estimates...
// static DWORD gStartTick;

// void ProgressCallback(double progress01, void* data)
// {
//   DWORD now = GetTickCount();
//   if (!gStartTick)
//     gStartTick = now;

//   double elapsedMs = double(now - gStartTick);

//   double remainingMs = 0.0;
//   if (progress01 > 0.0 && progress01 < 1.0)
//     remainingMs = (elapsedMs / progress01) - elapsedMs;

//   unsigned long elapsedSec = (unsigned long)(elapsedMs / 1000.0);
//   unsigned long remainingSec = (unsigned long)(remainingMs / 1000.0);

//   printf("\r%6.2f%%  %02lu:%02lu  -%02lu:%02lu   ",
//          progress01 * 100.0,
//          elapsedSec / 60,
//          elapsedSec % 60,
//          remainingSec / 60,
//          remainingSec % 60);

//   //fflush(stdout);
// }

// with time estimates and perf score (for profiling)...

struct ProgressContext
{
  DWORD startTick = 0;
  double songLengthSeconds = 0.0;
};
ProgressContext gProgressContext;

double GetSongLengthSeconds(const WaveSabrePlayerLib::SongRenderer::Song& song)
{
  WaveSabreCore::M7::Deserializer ds{(const uint8_t*)song.blob};
  ds.ReadUInt32();  // bpm
  return ds.ReadDouble();
}

// with time estimates and a reproducible perf score for optimization runs.
// Score is average render speed in "x realtime", so higher is better.
void ProgressCallback(double progress01, void* data)
{
  auto& context = gProgressContext;
  DWORD now = GetTickCount();

  double elapsedMs = double(now - context.startTick);
  double elapsedSeconds = elapsedMs / 1000.0;

  double remainingMs = 0.0;
  if (progress01 > 0.0 && progress01 < 1.0)
    remainingMs = (elapsedMs / progress01) - elapsedMs;

  double renderedSongSeconds = context.songLengthSeconds * progress01;
  double realtimeFactor = elapsedSeconds > 0.0 ? (renderedSongSeconds / elapsedSeconds) : 0.0;
  double msPerSongSecond = renderedSongSeconds > 0.0 ? (elapsedMs / renderedSongSeconds) : 0.0;

  unsigned long elapsedSec = (unsigned long)(elapsedMs / 1000.0);
  unsigned long remainingSec = (unsigned long)(remainingMs / 1000.0);

  printf("\r%6.2f%%  %02lu:%02lu  -%02lu:%02lu  %7.3fxRT  %7.1f ms/song-s",
         progress01 * 100.0,
         elapsedSec / 60,
         elapsedSec % 60,
         remainingSec / 60,
         remainingSec % 60,
         realtimeFactor,
         msPerSongSecond);
}



int main(int argc, char** argv)
{
  printf("\"Launching the rich into the sun\" by tenfour\nReleased at Revision 2026\n");
  if (argc != 2)
  {
    printf("Usage: %s path\\to\\file.wav\n", argv[0]);
  }
  else
  {
    const char* outputPath = argv[1];
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    // 1.5x number of processors is a good balance to ensure work is being done with every timeslice given.
    // testing showed this to be the point of diminishing returns on a 16 logical processor machine.
    // 4  @ 1m = 14:48, 2m = 10:29
    // 8  @ 1m = 13:31, 2m =  7:11
    // 12 @ 1m = 13:20, 2m =  6:27
    // 16 @ 1m = 12:59, 2m =  6:04
    // 24 @ 1m = 12:51, 2m =  5:58
    // 32 @ 1m = 12:55, 2m =  6:00
    // 48 @ 1m = 12:55, 2m =  5:58

    gProgressContext.songLengthSeconds = GetSongLengthSeconds(Song);
    gProgressContext.startTick = GetTickCount();  // reset start tick in case GetSongLengthSeconds took a long time.

    WaveSabrePlayerLib::WavWriter writer(&Song, (sysInfo.dwNumberOfProcessors * 3) >> 1);
    writer.Write(outputPath, ProgressCallback, nullptr);
  }
  printf("\n");
  ExitProcess(0);  // song renderer spawns a bunch of threads; this shuts down without having to coordinate threads.
}
