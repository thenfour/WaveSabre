#include <WaveSabreCore.h>
#include <WaveSabrePlayerLib.h>
using namespace WaveSabrePlayerLib;

#include <string.h>
#include "rendered.hpp"

void cout(const char* s) // assumes s is not a format string.
{
	printf(s);
}

void progressCallback(double progress, void *data)
{
	const int barLength = 32;
	int filledChars = (int)(progress * (double)(barLength - 1));
	cout("\r[");
	for (int j = 0; j < barLength; j++) putchar(filledChars >= j ? '*' : '-');
	cout("]");
}

int main(int argc, char **argv)
{
	::logl(1); // wut? this is needed to force the linker into pulling in logf() for some reason. why???
	if (argc <= 2) {
		cout("-w xyz.wav = output file\n-p         = play with prerender\n");
		return 0;
	}
	bool writeWav = argc >= 3 && !strcmp(argv[2], "-w");
	bool preRender = argc == 3 && !strcmp(argv[2], "-p");

	const int numRenderThreads = 3;



	SongRenderer::Song song;
	song.blob = SongBlob;
	song.factory = SongFactory;

	if (writeWav)
	{
		if (argc < 4) return 0; // did not specify filename
		WavWriter wavWriter(&song, numRenderThreads);

		cout("WAV writer activated. Rendering ...\n");
		wavWriter.Write(argv[3], progressCallback, nullptr);

		cout("\n\nDone.\n");
	}
	else
	{
		IPlayer *player;

		if (preRender)
		{
			cout("Precalc...\n\n");
			player = new PreRenderPlayer(&song, numRenderThreads, progressCallback, nullptr);
		}
		else
		{
			player = new RealtimePlayer(&song, numRenderThreads);
		}

		cout("Playing... ESC to quit.\n");
		player->Play();
		while (!GetAsyncKeyState(VK_ESCAPE))
		{
			auto songPos = player->GetSongPos();
			if (songPos >= player->GetLength()) break;
			int minutes = (int)songPos / 60;
			int seconds = (int)songPos % 60;
			int hundredths = (int)(songPos * 100.0) % 100;
			printf("\r %.1i:%.2i.%.2i", minutes, seconds, hundredths);

			Sleep(10);
		}
		cout("\n");
		delete player;
	}
	return 0;
}

