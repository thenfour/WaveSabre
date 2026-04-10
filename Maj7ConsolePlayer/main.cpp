#include <WaveSabrePlayerLib/WavWriter.h>
#include <WaveSabrePlayerLib/WaveOutPlayer.hpp>

//#include <WaveSabreCore/rendered.hpp>

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

double GetSongLengthSeconds(const WaveSabreCore::Song& song)
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
    printf("Playing. For wav writing, add the path to the cmd line.\n");
    // play the track, with 30 seconds of precalculation to give the system a chance to
    // catch up. for this prod should be more than enough.
    // create renderer and waveoutplayer.

    //WaveSabreCore::Song song;
    //song.blob = SongBlob;
    //song.factory = SongFactory;

    WSPlayerApp::Renderer renderer(GetConsoleWindow(), WaveSabreCore::gSong);
    WSPlayerApp::WaveOutPlayer player(renderer);

    renderer.Begin();

    DWORD precalcStart = GetTickCount();
    while (renderer.GetRenderStatus() != WSPlayerApp::Renderer::RenderStatus::Done)
    {
      printf("precalc ... %d \n", GetTickCount() - precalcStart);
      if (GetTickCount() - precalcStart >= WSPlayerApp::gMaxPrecalcMilliseconds)
      {
        break;
      }
      Sleep(2000);
    }

    player.PlayFrom(WSPlayerApp::WSTime::FromFrames(0));
    Sleep(renderer.gSongLength.GetMilliseconds() + 1000);
  }
  else
  {
    const char* outputPath = argv[1];

    //gProgressContext.songLengthSeconds = GetSongLengthSeconds(Song);
    //gProgressContext.startTick = GetTickCount();  // reset start tick in case GetSongLengthSeconds took a long time.

    // NB: 24 is the number of render threads.
    // optimal appears to be roughly 1.5x logical processor count.
    // probably depends on the song's graph topology; i'd actually prefer a fixed thread count based on the graph; a hard-coded graph plan.
    WaveSabrePlayerLib::WavWriter writer(&WaveSabreCore::gSong, 24);
    writer.Write(outputPath, ProgressCallback, nullptr);
  }
  printf("\n");
  ExitProcess(0);  // song renderer spawns a bunch of threads; this shuts down without having to coordinate threads.
}
