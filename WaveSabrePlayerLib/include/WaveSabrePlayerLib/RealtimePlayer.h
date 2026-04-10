#ifndef __WAVESABREPLAYERLIB_REALTIMEPLAYER_H__
#define __WAVESABREPLAYERLIB_REALTIMEPLAYER_H__

#include "IPlayer.h"
#include "SongRenderer.h"
#include "DirectSoundRenderThread.h"

namespace WaveSabrePlayerLib
{
	class RealtimePlayer : public IPlayer
	{
	public:
		explicit RealtimePlayer(int numRenderThreads, int bufferSizeMs = 1000);
		virtual ~RealtimePlayer();

		virtual void Play();
		
		//virtual int GetTempo() const;
		//virtual int GetSampleRate() const;
		//virtual double GetLength() const;
		virtual double GetSongPos() const;

	private:
		static void renderCallback(SongRenderer::Sample *buffer, int numSamples, void *data);

		const WaveSabreCore::Song *song;
		int numRenderThreads;
		int bufferSizeMs;

		SongRenderer *songRenderer;
		DirectSoundRenderThread *renderThread;
	};
}

#endif
