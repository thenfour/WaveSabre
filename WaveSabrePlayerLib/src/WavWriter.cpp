#include <WaveSabrePlayerLib/WavWriter.h>

#include <Windows.h>

namespace WaveSabrePlayerLib
{
	WavWriter::WavWriter(const SongRenderer::Song *song, int numRenderThreads)
	{
		songRenderer = new SongRenderer(song, numRenderThreads);
	}

	WavWriter::~WavWriter()
	{
		delete songRenderer;
	}

	void WavWriter::Write(const char *fileName, ProgressCallback callback, void *data)
	{
		//static constexpr int sampleRate = HARD_CODED_SAMPLE_RATE;// songRenderer->GetSampleRate();
		constexpr int bitsPerSample = sizeof(SongRenderer::Sample) * 8;

		constexpr int stepSize = 100 * SongRenderer::NumChannels;
    constexpr int z = WaveSabreCore::Helpers::CurrentSampleRateI * SongRenderer::NumChannels;
		int numSamples = (int)(z * songRenderer->GetLength()) / stepSize * stepSize;

		auto file = fopen(fileName, "wb");

		int dataSubChunkSize = numSamples * bitsPerSample / 8;
		
		// RIFF header
		fputs("RIFF", file);
		writeInt(36 + dataSubChunkSize, file);
		fputs("WAVE", file);

		// format subchunk
		fputs("fmt ", file);
		writeInt(16, file);
		writeShort(WAVE_FORMAT_PCM, file);
		writeShort(SongRenderer::NumChannels, file);
    writeInt(WaveSabreCore::Helpers::CurrentSampleRateI, file);
    writeInt(WaveSabreCore::Helpers::CurrentSampleRateI * SongRenderer::NumChannels * bitsPerSample / 8, file);
		writeShort(SongRenderer::NumChannels * bitsPerSample / 8, file);
		writeShort(bitsPerSample, file);

		// data subchunk
		fputs("data", file);
		writeInt(dataSubChunkSize, file);

		int stepCounter = 0;

		SongRenderer::Sample buf[stepSize];
		for (int i = 0; i < numSamples; i += stepSize)
		{
			songRenderer->RenderSamples(buf, stepSize);
			for (int j = 0; j < stepSize; j++) writeShort(buf[j], file);

			stepCounter--;
			if (stepCounter <= 0)
			{
				if (callback)
				{
					double progress = (double)i / (double)numSamples;
					callback(progress, data);
				}
				stepCounter = 200;
			}
		}

		fclose(file);

		if (callback)
			callback(1.0, data);
	}

	void WavWriter::writeInt(int i, FILE *file)
	{
		fwrite(&i, sizeof(int), 1, file);
	}

	void WavWriter::writeShort(short s, FILE *file)
	{
		fwrite(&s, sizeof(short), 1, file);
	}
}
