#include <WaveSabreCore.h>
#include <WaveSabrePlayerLib.h>
using namespace WaveSabrePlayerLib;

#include <string.h>
#include "rendered.hpp"

void progressCallback(double progress, void *data)
{
	const int barLength = 32;
	int filledChars = (int)(progress * (double)(barLength - 1));
	printf("\r[");
	for (int j = 0; j < barLength; j++) putchar(filledChars >= j ? '*' : '-');
	printf("]");
}

int main(int argc, char **argv)
{
	bool writeWav = argc >= 3 && !strcmp(argv[2], "-w");
	bool preRender = argc == 3 && !strcmp(argv[2], "-p");

	const int numRenderThreads = 3;

	SongRenderer::Song song;
	song.blob = SongBlob;
	song.factory = SongFactory;

	if (writeWav)
	{
		WavWriter wavWriter(&song, numRenderThreads);

		printf("WAV writer activated.\n");

		auto fileName = argc >= 4 ? argv[3] : "out.wav";
		printf("Rendering...\n");
		wavWriter.Write(fileName, progressCallback, nullptr);

		printf("\n\nWAV file written to \"%s\"\n", fileName);
	}
	else
	{
		IPlayer *player;

		if (preRender)
		{
			printf("Prerender running...\n\n");
			player = new PreRenderPlayer(&song, numRenderThreads, progressCallback, nullptr);
		}
		else
		{
			player = new RealtimePlayer(&song, numRenderThreads);
		}

		printf("Playing... ESC to quit.\n");
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
		printf("\n");
		delete player;
	}
	return 0;
}

