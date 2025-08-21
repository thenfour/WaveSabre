#include <WaveSabrePlayerLib/PreRenderPlayer.h>

namespace WaveSabrePlayerLib
{
	PreRenderPlayer::PreRenderPlayer(const SongRenderer::Song *song, int numRenderThreads, ProgressCallback callback, void *data, int playbackBufferSizeMs)
	{
		SongRenderer songRenderer(song, numRenderThreads);

		//tempo = songRenderer.GetTempo();
		//sampleRate = songRenderer.GetSampleRate();
		length = songRenderer.GetLength();

		constexpr int stepSize = 100 * SongRenderer::NumChannels;
#ifdef MIN_SIZE_REL
		constexpr int z = WaveSabreCore::Helpers::CurrentSampleRateI * SongRenderer::NumChannels;
#else 
		const int z = WaveSabreCore::Helpers::CurrentSampleRateI * SongRenderer::NumChannels;
#endif  // MIN_SIZE_REL
		renderBufferSize = (int)(z * songRenderer.GetLength()) / stepSize * stepSize;
		renderBuffer = new SongRenderer::Sample[renderBufferSize];

		int stepCounter = 0;

		for (int i = 0; i < renderBufferSize; i += stepSize)
		{
			songRenderer.RenderSamples(renderBuffer + i, stepSize);

			stepCounter--;
			if (stepCounter <= 0)
			{
				if (callback)
				{
					double progress = (double)i / (double)renderBufferSize;
					callback(progress, data);
				}
				stepCounter = 200;
			}
		}

		if (callback)
			callback(1.0, data);

		this->playbackBufferSizeMs = playbackBufferSizeMs;

		renderThread = nullptr;
	}

	PreRenderPlayer::~PreRenderPlayer()
	{
		if (renderThread)
			delete renderThread;

		delete [] renderBuffer;
	}

	void PreRenderPlayer::Play()
	{
		if (renderThread)
			delete renderThread;

		playbackBufferIndex = 0;

		renderThread = new DirectSoundRenderThread(renderCallback, this, WaveSabreCore::Helpers::CurrentSampleRateI, playbackBufferSizeMs);
	}

	//int PreRenderPlayer::GetTempo() const
	//{
	//	return tempo;
	//}

	//int PreRenderPlayer::GetSampleRate() const
	//{
	//	return sampleRate;
	//}

	double PreRenderPlayer::GetLength() const
	{
		return length;
	}

	double PreRenderPlayer::GetSongPos() const
	{
		if (!renderThread)
			return 0.0;

		return max((renderThread->GetPlayPositionMs() - (double)playbackBufferSizeMs) / 1000.0, 0.0);
	}

	void PreRenderPlayer::renderCallback(SongRenderer::Sample *buffer, int numSamples, void *data)
	{
		auto player = (PreRenderPlayer *)data;

		int samplesLeft = player->renderBufferSize - player->playbackBufferIndex;
		if (!samplesLeft)
		{
			memset(buffer, 0, numSamples * sizeof(SongRenderer::Sample));
			return;
		}

		int samplesToTake = min(numSamples, samplesLeft);
		if (samplesToTake)
		{
			memcpy(buffer, player->renderBuffer + player->playbackBufferIndex, samplesToTake * sizeof(SongRenderer::Sample));
			player->playbackBufferIndex += samplesToTake;
		}
		if (samplesToTake < numSamples)
		{
			int remainingSamples = numSamples - samplesToTake;
			memset(buffer + samplesToTake, 0, remainingSamples * sizeof(SongRenderer::Sample));
		}
	}
}
