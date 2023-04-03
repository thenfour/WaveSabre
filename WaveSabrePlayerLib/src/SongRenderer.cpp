#include <WaveSabrePlayerLib/SongRenderer.h>

using namespace WaveSabreCore;

namespace WaveSabrePlayerLib
{
	//GraphRunnerProfiler* gpGraphProfiler = nullptr;

	SongRenderer::SongRenderer(const SongRenderer::Song *song, int numRenderThreads)
	{
		//Helpers::Init();
		//gpGraphProfiler = new GraphRunnerProfiler(numRenderThreads);

		songBlobPtr = song->blob;

		bpm = readInt();
		sampleRate = readInt();
		length = readDouble();

		numDevices = readInt();
		devices = new Device *[numDevices];
		for (int i = 0; i < numDevices; i++)
		{
			devices[i] = song->factory((DeviceId)readByte());
			devices[i]->SetSampleRate((float)sampleRate);
			devices[i]->SetTempo(bpm);
			int chunkSize = readInt();
			devices[i]->SetChunk((void *)songBlobPtr, chunkSize);
			songBlobPtr += chunkSize;
		}

		numMidiLanes = readInt();
		midiLanes = new MidiLane *[numMidiLanes];
		for (int i = 0; i < numMidiLanes; i++)
		{
			midiLanes[i] = new MidiLane;
			int numEvents = readInt();
			midiLanes[i]->numEvents = numEvents;
			midiLanes[i]->events = new Event[numEvents];

			for (int m = 0; m < numEvents; m++)
			{
				midiLanes[i]->events[m].TimeStamp = readInt();
			}

			for (int m = 0; m < numEvents; m++)
			{
				byte event = readByte();
				midiLanes[i]->events[m].Type = (EventType)event;
			}

			for (int m = 0; m < numEvents; m++)
			{
				midiLanes[i]->events[m].Note = readByte();
			}

			for (int m = 0; m < numEvents; m++)
			{
				midiLanes[i]->events[m].Velocity = readByte();
			}
		}

		numTracks = readInt();
		tracks = new Track *[numTracks];
		trackRenderStates = new TrackRenderState[numTracks];
		for (int i = 0; i < numTracks; i++)
		{
			tracks[i] = new Track(this, song->factory);
			trackRenderStates[i] = TrackRenderState::Finished;
		}

		this->numRenderThreads = numRenderThreads;

		renderThreadShutdown = false;
		renderThreadStartEvents = new HANDLE[numRenderThreads];
		for (int i = 0; i < numRenderThreads; i++)
			renderThreadStartEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		renderDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		if (numRenderThreads > 1)
		{
			additionalRenderThreads = new HANDLE[numRenderThreads - 1];
			for (int i = 0; i < numRenderThreads - 1; i++)
			{
				auto renderThreadData = new RenderThreadData();
				renderThreadData->songRenderer = this;
				renderThreadData->renderThreadIndex = i + 1;
				additionalRenderThreads[i] = CreateThread(0, 0, renderThreadProc, (LPVOID)renderThreadData, 0, 0);

				// on one hand this pulls in a DLL import, on the other hand a few bytes of text is trivial and this helps us get down to the precalc requirement.
				//SetThreadPriority(additionalRenderThreads[i], THREAD_PRIORITY_HIGHEST);
				SetThreadPriority(additionalRenderThreads[i], THREAD_PRIORITY_ABOVE_NORMAL);
			}
		}
	}

	SongRenderer::~SongRenderer()
	{
		// Dispatch shutdown
		renderThreadShutdown = true;

		if (numRenderThreads > 1)
		{
			for (int i = 0; i < numRenderThreads; i++)
				SetEvent(renderThreadStartEvents[i]);
			WaitForMultipleObjects(numRenderThreads - 1, additionalRenderThreads, TRUE, INFINITE);
			for (int i = 0; i < numRenderThreads - 1; i++)
				CloseHandle(additionalRenderThreads[i]);
			delete [] additionalRenderThreads;
		}

		for (int i = 0; i < numDevices; i++) delete devices[i];
		delete [] devices;

		for (int i = 0; i < numMidiLanes; i++) {
			delete midiLanes[i]->events;
			delete midiLanes[i];
		}
		delete [] midiLanes;

		for (int i = 0; i < numTracks; i++) delete tracks[i];
		delete [] tracks;
		delete [] trackRenderStates;

		for (int i = 0; i < numRenderThreads; i++)
			CloseHandle(renderThreadStartEvents[i]);
		CloseHandle(renderDoneEvent);

		delete [] renderThreadStartEvents;
	}

	void SongRenderer::RenderSamples(Sample *buffer, int numSamples)
	{
		MxcsrFlagGuard mxcsrFlagGuard;

		// Set up work
		for (int i = 0; i < numTracks; i++)
			trackRenderStates[i] = TrackRenderState::Idle;
		renderThreadNumFloatSamples = numSamples / 2;

		// Dispatch work
		//gpGraphProfiler->BeginGraph();
		renderThreadsRunning = numRenderThreads;
		for (int i = 0; i < numRenderThreads; i++)
			SetEvent(renderThreadStartEvents[i]);

		renderThreadWork(0);

		// Wait for render threads to complete their work
		WaitForSingleObject(renderDoneEvent, INFINITE);
		//gpGraphProfiler->EndGraph();

		// Copy final output
		float **masterTrackBuffers = tracks[numTracks - 1]->Buffers;
		for (int i = 0; i < numSamples; i++)
		{
			int sample = (int)(masterTrackBuffers[i & 1][i / 2] * 32767.0f);
			if (sample < -32768) sample = -32768;
			if (sample > 32767) sample = 32767;
			buffer[i] = (Sample)sample;
		}
	}

	DWORD WINAPI SongRenderer::renderThreadProc(LPVOID lpParameter)
	{
		auto renderThreadData = (RenderThreadData *)lpParameter;

		auto songRenderer = renderThreadData->songRenderer;
		int renderThreadIndex = renderThreadData->renderThreadIndex;

		delete renderThreadData;

		while (true)
		{
			WaitForSingleObject(songRenderer->renderThreadStartEvents[renderThreadIndex], INFINITE);
			if (songRenderer->renderThreadShutdown)
				break;
			songRenderer->renderThreadWork(renderThreadIndex);
		}

		return 0;
	}

	bool SongRenderer::renderThreadWork(int renderThreadIndex)
	{
		MxcsrFlagGuard mxcsrFlagGuard;

		// Check that there's _any_ potential work to be done
		// this is an odd way to check if there's any work left. well they are taken sequentially; there will never be a situation
		// where track 2 is finished but track 1 hasn't been selected for processing yet.
		// it's not 100% accurate though because if track 2 is NOT finished yet, but track 1 IS finished, then we're going to spin too much.
		// it would be more accurate to have an atomic counter like "# of tracks to be processed". but then like,
		// why not just TRY to pick up work and if you couldn't find anything then exit?
		// hm i still don't think this is actually reliable. maybe it's that the master track is always the last one or something?
		// it could be that a bunch of tracks depend on the last track, so they are skipped, the last track gets picked up and finished,
		// and then all the threads disappear before dependent tracks are processed.
		
		// therefore, 
		// TODO: use a sync primitive to look for work, and upon seeing all tracks finished, then exit only.
		while (trackRenderStates[numTracks - 1] != TrackRenderState::Finished)
		{
			// find a track that is ready to be processed.
			// TODO: is there a race condition here? if multiple threads find the same available track...
			// i guess this is bound to happen but the InterlockedCompareExchange will fail to work.
			// 
			// should we instead do this in the main thread and dispatch work to the pool?
			for (int i = 0; i < numTracks; i++)
			{
				if (trackRenderStates[i] != TrackRenderState::Idle)// If track is being worked, skip it
				{
					continue;
				}
				// If any of the track's dependencies aren't finished, skip it
				bool allDependenciesFinished = true;
				for (int j = 0; j < tracks[i]->NumReceives; j++)
				{
					if (trackRenderStates[tracks[i]->Receives[j].SendingTrackIndex] != TrackRenderState::Finished)
					{
						allDependenciesFinished = false;
						break;
					}
				}
				if (!allDependenciesFinished) {
					continue;
				}

				// We have a free track that we can work on, yay!
				//  Let's try to mark it so that no other thread takes it
				if ((TrackRenderState)InterlockedCompareExchange((unsigned int *)&trackRenderStates[i], (unsigned int)TrackRenderState::Rendering, (unsigned int)TrackRenderState::Idle) == TrackRenderState::Idle)
				{
					//Stopwatch sw;
					//gpGraphProfiler->BeginWork(sw);
					tracks[i]->Run(renderThreadNumFloatSamples);
					//gpGraphProfiler->EndWork(sw);
					trackRenderStates[i] = TrackRenderState::Finished;
					break;
				}
			}

			// TODO: check that all tracks were finished and exit if so.
			// use a sync primitive to WAIT until any track was finished; then loop.
			Sleep(0);
		}

		if (!InterlockedDecrement(&renderThreadsRunning))
			SetEvent(renderDoneEvent);

		return true;
	}

	int SongRenderer::GetTempo() const
	{
		return bpm;
	}

	int SongRenderer::GetSampleRate() const
	{
		return sampleRate;
	}

	double SongRenderer::GetLength() const
	{
		return length;
	}

	unsigned char SongRenderer::readByte()
	{
		unsigned char ret = *songBlobPtr;
		songBlobPtr++;
		return ret;
	}

	int SongRenderer::readInt()
	{
		int ret = *(int *)songBlobPtr;
		songBlobPtr += sizeof(int);
		return ret;
	}

	float SongRenderer::readFloat()
	{
		float ret = *(float *)songBlobPtr;
		songBlobPtr += sizeof(float);
		return ret;
	}

	double SongRenderer::readDouble()
	{
		double ret = *(double *)songBlobPtr;
		songBlobPtr += sizeof(double);
		return ret;
	}
}
