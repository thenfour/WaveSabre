// assumptions: LAST track is the master track.

#pragma once

#include <new>     // for placement new
#include <WaveSabreCore.h>
#include "SongRenderer.h"


namespace WaveSabrePlayerLib
{
	struct GraphProcessor
	{
		enum class NodeStatus
		{
			Waiting,
			ReadyToBePickedUp,
			Processing,
			Finished,
		};
		struct INode
		{
			NodeStatus INode_NodeStatus = NodeStatus::Waiting; // populated by graph
			int INode_NodesDependingOnThis = 0; // populated by graph

			virtual void INode_Run(int numSamples) = 0;
			int INode_DirectDependencyCount = 0; // populated by child class (num receives)
			virtual int INode_GetDependencyIndex(int index) const = 0;
		};
		struct INodeList
		{
			int INodeList_NodeCount = 0; // populated by child class
			virtual INode* INodeList_GetNode(int i) = 0;
		};

		const int mThreadCount;
		INodeList* const mNodeList = nullptr;

		HANDLE* mThreads = nullptr;
		HANDLE mEvents[2];
#define hWorkAvailableEvent mEvents[0]
#define hGraphCompleteEvent mEvents[1]

		void populateNodesDependingOnThis_dfs(int nodeIndex)
		{
			INode& node = *mNodeList->INodeList_GetNode(nodeIndex);
			for (int i = 0; i < node.INode_DirectDependencyCount; i++)
			{
				int dependencyIndex = node.INode_GetDependencyIndex(i);
				mNodeList->INodeList_GetNode(dependencyIndex)->INode_NodesDependingOnThis++;
				populateNodesDependingOnThis_dfs(dependencyIndex);
			}
		}

		// Function to populate INode_NodesDependingOnThis for each node in the graph, hierarchically
		void populateNodesDependingOnThis()
		{
			// already initialized. could assert this.
			//for (int i = 0; i < mNodeList->INodeList_NodeCount; i++)
			//{
			//	mNodeList->INodeList_GetNode(i)->INode_NodesDependingOnThis = 0;
			//}

			for (int i = 0; i < mNodeList->INodeList_NodeCount; i++)
			{
				populateNodesDependingOnThis_dfs(i);
				//INode& node = *mNodeList->INodeList_GetNode(i);
				//for (int j = 0; j < node.INode_DirectDependencyCount; j++)
				//{
				//	int dependencyIndex = node.INode_GetDependencyIndex(j);
				//	mNodeList->INodeList_GetNode(dependencyIndex)->INode_NodesDependingOnThis++;
				//	populateNodesDependingOnThis_dfs(dependencyIndex);
				//}
			}
		}

		WaveSabreCore::CriticalSection mStateLock;

		GraphProcessor(int numThreads, INodeList* nodeList) :
			mThreadCount(numThreads),
			mNodeList(nodeList)
		{
			populateNodesDependingOnThis();

			for (int i = 0; i < 2; ++i) {
				mEvents[i] = CreateEvent(0, FALSE, FALSE, 0);
			}

			// the calling thread counts but doesn't require a handle. allocating 1 extra saves some code size.
			mThreads = new HANDLE[numThreads];
			int ntm1 = numThreads - 1;
			for (int i = 0; i < ntm1; i++)
			{
				mThreads[i] = CreateThread(0, 0, renderThreadProc, this, 0, 0);
				// on one hand this pulls in a DLL import, on the other hand a few bytes of text is trivial and this helps us get down to the precalc requirement.
				//SetThreadPriority(mThreads[i], THREAD_PRIORITY_HIGHEST);
				SetThreadPriority(mThreads[i], THREAD_PRIORITY_ABOVE_NORMAL);
			}
		}

		~GraphProcessor()
		{
			// todo
#pragma message("GraphProcessor::~GraphProcessor() Leaking memory to save bits.")
		}

		// when attempting to accept work, the result can be that there's just no work to be done, etc.
		struct AcceptWorkResult
		{
			INode* mAcceptedWork = nullptr; // non-null and status updated if accepted
			int mAdditionalNodesReadyToWorkCount = 0; // does not include the one that you just accepted.
			bool HasWork() { return !!mAcceptedWork; }
			void DoTheWork(int numSamples) {
				mAcceptedWork->INode_Run(numSamples);
			}
		};

		// variable for workers
		int mNumFrames = 0;

		std::atomic<int> mNodesToBeProcessed = 0;

		// the "main thread" or caller.
		void ProcessGraph(int numSamples)
		{
			ResetEvent(hWorkAvailableEvent);

			//gpGraphProfiler->BeginGraph();
			mNumFrames = numSamples / 2;
			for (int iNode = 0; iNode < mNodeList->INodeList_NodeCount; ++iNode)
			{
				mNodeList->INodeList_GetNode(iNode)->INode_NodeStatus = NodeStatus::Waiting;
			}
			mNodesToBeProcessed = mNodeList->INodeList_NodeCount;

			// workers all want to wait for work.
			auto work = MarkWorkDone_Dispatch_AcceptNewWork({});
			GraphWorkerLoop(work, true);

			//gpGraphProfiler->EndGraph();
		}

		void WorkerThread()
		{
			while(true) GraphWorkerLoop({}, false);
		}

		// exits when the entire graph has been completed.
		// todo: return bool if we're shutting down threads
		void GraphWorkerLoop(AcceptWorkResult work, bool breakWhenGraphComplete)
		{
			// the main thread is a bit different because it needs to be alerted when the graph is complete.
			while (mNodesToBeProcessed)
			{
				if (!work.HasWork()) {
					work = WaitAndAcceptWork(breakWhenGraphComplete); // if work didn't come from the previous loop
				}
				if (!work.HasWork()) {
					continue; // it's theoretically possible that we heard about work being available but when we looked for it none was to be found. just go back to waiting.
				}
				WaveSabreCore::MxcsrFlagGuard mxcsrFlagGuard;
				//Stopwatch sw;
				//gpGraphProfiler->BeginWork(sw);
				work.DoTheWork(mNumFrames);
				//gpGraphProfiler->EndWork(sw);
				work = MarkWorkDone_Dispatch_AcceptNewWork(work);
			}
			SetEvent(hGraphCompleteEvent);
		}

		// accepts work, but waits if needed.
		AcceptWorkResult WaitAndAcceptWork(bool breakWhenGraphComplete)
		{
			if (breakWhenGraphComplete) {
				//HANDLE h[2] = { hWorkAvailableEvent , hGraphCompleteEvent };
				WaitForMultipleObjects(2, mEvents, FALSE, INFINITE); // either event results in the same action
			}
			else {
				WaitForSingleObject(hWorkAvailableEvent, INFINITE);
			}
			if (!mNodesToBeProcessed) {
				return {}; // graph complete.
			}

			// it may be that another thread picked up the work before this lock is possible.
			// in that case maybe no work is accepted, even if work was available. just know that this can happen.
			auto lock = mStateLock.Enter();
			return UpdateAndScanGraphForWork();
		}

		// this is the routine that scans all graph,
		// - accepts the highest priority task
		// - convert waiting tracks to ready_for_processing
		// WARNING: ASSUMES a lock has been acquired for states. Caller do this.
		AcceptWorkResult UpdateAndScanGraphForWork()
		{
			// dispatch work if there are any available.
			// that means taking any waiting work and converting to ready/queued
			AcceptWorkResult r;
			int highestPrioAccepted = -1;
			for (int iNode = 0; iNode < mNodeList->INodeList_NodeCount; ++iNode)
			{
				auto& node = *mNodeList->INodeList_GetNode(iNode);

				if (node.INode_NodeStatus == NodeStatus::Waiting)
				{
					// look at dependencies; can we promote to ReadyToBePickedUp?
					bool allClear = true;
					for (int iDep = 0; iDep < node.INode_DirectDependencyCount; ++iDep) {
						auto idepnode = node.INode_GetDependencyIndex(iDep);
						if (mNodeList->INodeList_GetNode(idepnode)->INode_NodeStatus != NodeStatus::Finished) {
							allClear = false;
							break;
						}
					}
					if (allClear) {
						node.INode_NodeStatus = NodeStatus::ReadyToBePickedUp;
					}
				}

				if (node.INode_NodeStatus == NodeStatus::ReadyToBePickedUp)
				{
					if (node.INode_NodesDependingOnThis > highestPrioAccepted) {
						highestPrioAccepted = node.INode_NodesDependingOnThis;
					//if (!r.mAcceptedWork) {
						r.mAcceptedWork = &node;
					}
					r.mAdditionalNodesReadyToWorkCount++;
				}

			} // for each node

			if (r.HasWork()) {
				// caller is accepting a node for work. prepare that state.
				r.mAcceptedWork->INode_NodeStatus = NodeStatus::Processing;
				-- r.mAdditionalNodesReadyToWorkCount; // deduct the one the caller is accepting.
			}

			return r;
		}

		// Q: why a weird function that does 3 things? to avoid locking any more than i need to.
		AcceptWorkResult MarkWorkDone_Dispatch_AcceptNewWork(AcceptWorkResult doneWork)
		{
			AcceptWorkResult r;
			{
				auto lock = mStateLock.Enter();
				if (doneWork.HasWork())
				{
					doneWork.mAcceptedWork->INode_NodeStatus = NodeStatus::Finished;
					--mNodesToBeProcessed;
				}
				if (!mNodesToBeProcessed)
				{
					return {}; // graph complete.
				}

				r = UpdateAndScanGraphForWork();
			}
			for (int i = 0; i < r.mAdditionalNodesReadyToWorkCount; ++i)
			{
				SetEvent(hWorkAvailableEvent);
			}
			return r;
		}

		static DWORD WINAPI renderThreadProc(LPVOID lpParameter)
		{
			((GraphProcessor*)lpParameter)->WorkerThread();
			return 0;
		}

	}; // class GraphProcessor

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SongRenderer2 : GraphProcessor::INodeList
	{
		using Song = SongRenderer::Song;
		using DeviceId = SongRenderer::DeviceId;
		using DeviceFactory = SongRenderer::DeviceFactory;

		using EventType = SongRenderer::EventType;
		using Event = SongRenderer::Event;
		using Devices = SongRenderer::Devices;
		using MidiLane = SongRenderer::MidiLane;

		typedef int16_t Sample;

		static constexpr int NumChannels = 2;
		static constexpr int BitsPerSample = 16;
		static constexpr int BlockAlign = NumChannels * BitsPerSample / 8;

		struct Track : GraphProcessor::INode
		{
			typedef struct
			{
				int SendingTrackIndex;
				int ReceivingChannelIndex;
				float Volume;
			} Receive;

			Track(SongRenderer2* songRenderer, DeviceFactory factory, WaveSabreCore::M7::Deserializer& ds)
			{
				this->songRenderer = songRenderer;

				for (int i = 0; i < numBuffers; i++) {
					Buffers[i] = new float[songRenderer->sampleRate];
				}

				volume = ds.ReadFloat();

				INode_DirectDependencyCount = NumReceives = ds.ReadVarUInt32();
				if (NumReceives)
				{
					Receives = new Receive[NumReceives];
					for (int i = 0; i < NumReceives; i++)
					{
						auto& r = Receives[i];
						r.SendingTrackIndex = ds.ReadVarUInt32();
						r.ReceivingChannelIndex = ds.ReadVarUInt32();
						r.Volume = ds.ReadFloat();
					}
				}

				numDevices = ds.ReadVarUInt32();
				if (numDevices)
				{
					devicesIndicies = new int[numDevices];
					for (int i = 0; i < numDevices; i++)
					{
						devicesIndicies[i] = ds.ReadVarUInt32();
					}
				}

				midiLaneId = ds.ReadVarUInt32();

				numAutomations = ds.ReadVarUInt32();
				if (numAutomations)
				{
					automations = new Automation * [numAutomations];
					for (int i = 0; i < numAutomations; i++)
					{
						int deviceIndex = ds.ReadVarUInt32();
						automations[i] = new Automation(songRenderer, songRenderer->devices[devicesIndicies[deviceIndex]], ds);
					}
				}

				lastSamplePos = 0;
				accumEventTimestamp = 0;
				eventIndex = 0;
			}
			~Track()
			{
#pragma message("SongRenderer2::Track::~Track() Leaking memory to save bits.")
				//for (int i = 0; i < numBuffers; i++) delete[] Buffers[i];

				//if (NumReceives)
				//	delete[] Receives;

				//if (numDevices)
				//{
				//	delete[] devicesIndicies;
				//}

				//if (numAutomations)
				//{
				//	for (int i = 0; i < numAutomations; i++) delete automations[i];
				//	delete[] automations;
				//}
			}

			virtual int INode_GetDependencyIndex(int index) const override
			{
				return this->Receives[index].SendingTrackIndex;
			}

			virtual void INode_Run(int numSamples) override
			{
				MidiLane& lane = songRenderer->midiLanes[midiLaneId];
				for (; eventIndex < lane.numEvents; eventIndex++)
				{
					Event& e = lane.events[eventIndex];
					int samplesToEvent = accumEventTimestamp + e.TimeStamp - lastSamplePos;
					if (samplesToEvent >= numSamples) break;
					for (int i = 0; i < numDevices; i++) {
						auto pd = songRenderer->devices[devicesIndicies[i]];
						switch (e.Type)
						{
						case EventType::NoteOn:
							pd->NoteOn(e.Note, e.Velocity, samplesToEvent);
							break;
						case EventType::NoteOff:
							pd->NoteOff(e.Note, samplesToEvent);
							break;
						case EventType::CC:
							pd->MidiCC(e.Note, e.Velocity, samplesToEvent);
							break;
						case EventType::PitchBend:
							pd->PitchBend(e.Note, e.Velocity, samplesToEvent);
							break;
						}
					}
					accumEventTimestamp += e.TimeStamp;
				}

				for (int i = 0; i < numAutomations; i++) automations[i]->Run(numSamples);

				for (int i = 0; i < numBuffers; i++) memset(Buffers[i], 0, numSamples * sizeof(float));
				for (int i = 0; i < NumReceives; i++)
				{
					Receive* r = &Receives[i];
					float** receiveBuffers = songRenderer->tracks[r->SendingTrackIndex].Buffers;
					for (int j = 0; j < 2; j++)
					{
						for (int k = 0; k < numSamples; k++) {
							Buffers[j + r->ReceivingChannelIndex][k] += receiveBuffers[j][k] * r->Volume;
						}
					}
				}

				for (int i = 0; i < numDevices; i++) {
					songRenderer->devices[devicesIndicies[i]]->Run((double)lastSamplePos / WaveSabreCore::Helpers::CurrentSampleRate, Buffers, Buffers, numSamples);
				}

				if (volume != 1.0f)
				{
					for (int i = 0; i < numBuffers; i++)
					{
						for (int j = 0; j < numSamples; j++) Buffers[i][j] *= volume;
					}
				}

				lastSamplePos += numSamples;
			}

		private:
			static const int numBuffers = 4;
		public:
			float* Buffers[numBuffers];

			int NumReceives;
			Receive* Receives;

		private:
			class Automation
			{
			public:
				Automation(SongRenderer2* songRenderer, WaveSabreCore::Device* device, WaveSabreCore::M7::Deserializer& ds)
				{
					this->device = device;
					paramId = ds.ReadVarUInt32();
					numPoints = ds.ReadVarUInt32();
					points = new Point[numPoints];
					int lastPointTime = 0;
					for (int i = 0; i < numPoints; i++)
					{
						int absTime = lastPointTime + ds.ReadVarUInt32();
						auto& p = points[i];
						p.TimeStamp = absTime;
						lastPointTime = absTime;
						p.Value = (float)ds.ReadUByte() / 255.0f;
					}
					samplePos = 0;
					pointIndex = 0;
				}
				~Automation()
				{
					delete[] points;
				}

				void Run(int numSamples)
				{
					for (; pointIndex < numPoints; pointIndex++)
					{
						if (points[pointIndex].TimeStamp > samplePos) break;
					}
					if (pointIndex >= numPoints)
					{
						device->SetParam(paramId, points[numPoints - 1].Value);
					}
					else if (pointIndex <= 0)
					{
						device->SetParam(paramId, points[0].Value);
					}
					else
					{
						auto& p0 = points[pointIndex];
						auto& pm1 = points[pointIndex - 1];
						int timestampDelta = p0.TimeStamp - pm1.TimeStamp;
						float mixAmount = timestampDelta > 0 ?
							(float)(samplePos - pm1.TimeStamp) / (float)timestampDelta :
							0.0f;
						device->SetParam(paramId, WaveSabreCore::M7::math::lerp(pm1.Value, p0.Value, mixAmount));
					}
					samplePos += numSamples;
				}

			private:
				typedef struct
				{
					int TimeStamp;
					float Value;
				} Point;

				WaveSabreCore::Device* device;
				int paramId;

				int numPoints;
				Point* points;

				int samplePos;
				int pointIndex;
			};

			SongRenderer2* songRenderer;

			float volume;

			int numDevices;
			int* devicesIndicies;

			int midiLaneId;

			int numAutomations;
			Automation** automations;

			int lastSamplePos;
			int accumEventTimestamp;
			int eventIndex;
		}; // class track


		SongRenderer2(const Song* song, int numRenderThreads)
		{
			WaveSabreCore::M7::Deserializer ds{ (const uint8_t*)song->blob };

			ds.ReadUInt32(); // assert(r = WSBR)
			ds.ReadUInt32(); // assert 4-byte version

			bpm = ds.ReadUInt32();
			sampleRate = ds.ReadUInt32();
			length = ds.ReadDouble();

			numDevices = ds.ReadUInt32();
			devices = new WaveSabreCore::Device * [numDevices];
			for (int i = 0; i < numDevices; i++)
			{
				auto& d = devices[i];
				d = song->factory((DeviceId)ds.ReadUByte());
				d->SetSampleRate((float)sampleRate);
				d->SetTempo(bpm);
				int chunkSize = ds.ReadVarUInt32();
				d->SetChunk((void*)ds.mpCursor, chunkSize);
				ds.mpCursor += chunkSize;
			}

			numMidiLanes = ds.ReadUInt32();
			midiLanes = new MidiLane[numMidiLanes];
			for (int i = 0; i < numMidiLanes; i++)
			{
				bool fixedVelocity = !!ds.ReadUByte();
				int numEvents = ds.ReadUInt32();
				auto& midiLane = midiLanes[i];
				midiLane.numEvents = numEvents;
				midiLane.events = new Event[numEvents];

				for (int m = 0; m < numEvents; m++)
				{
					auto& e = midiLane.events[m];
					auto t = ds.ReadVarUInt32();
					e.Type = (EventType)(t & 3);
					e.TimeStamp = t >> 2;
				}

				for (int m = 0; m < numEvents; m++)
				{
					midiLane.events[m].Note = ds.ReadUByte();
				}

				for (int m = 0; m < numEvents; m++)
				{
					auto& e = midiLane.events[m];
					e.Velocity = 100;
					switch (e.Type) {
					case EventType::NoteOff:
						continue;
					case EventType::NoteOn:
						if (fixedVelocity)
							continue;
						break;
					}
					e.Velocity = ds.ReadUByte();
				}
			}

			numTracks = ds.ReadUInt32();
			this->INodeList_NodeCount = numTracks;

			this->tracks = (Track*)malloc(sizeof(Track) * numTracks);
			for (int i = 0; i < numTracks; i++)
			{
				new (this->tracks + i) Track(this, song->factory, ds);
			}

			// it's interesting to just create a thread for each track and let the system schedule (and therefore less synchronization in our threads). but it's not more efficient.
			mpGraphRunner = new GraphProcessor(/*numTracks*/numRenderThreads, this);
		}

		void RenderSamples(Sample* buffer, int numSamples)
		{
			mpGraphRunner->ProcessGraph(numSamples);

			// Copy final output
			float** masterTrackBuffers = tracks[numTracks - 1].Buffers;
			for (int i = 0; i < numSamples; i++)
			{
				buffer[i] = WaveSabreCore::M7::math::Sample32To16(masterTrackBuffers[i & 1][i >> 1]);
			}
		}

		int GetTempo() const
		{
			return bpm;
		}
		int GetSampleRate() const
		{
			return sampleRate;
		}
		double GetLength() const
		{
			return length;
		}


		GraphProcessor* mpGraphRunner = nullptr;

		int songDataIndex;

		int bpm;
		int sampleRate;
		double length;

		int numDevices;
		WaveSabreCore::Device** devices;

		int numMidiLanes;
		MidiLane* midiLanes;

		int numTracks;
		Track* tracks;

		virtual GraphProcessor::INode* INodeList_GetNode(int i) override {
			return &tracks[i];
		}

	}; // class SongRenderer2
} // namespace WaveSabrePlayerLib

