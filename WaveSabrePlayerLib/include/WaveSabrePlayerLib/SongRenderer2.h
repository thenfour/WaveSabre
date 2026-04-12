#pragma once

#include <new>     // for placement new
#include <WaveSabreCore.h>

namespace WaveSabrePlayerLib
{
	struct GraphProcessor2
	{
		struct INode
		{
			virtual void INode_Run(int numSamples) = 0;
		};
		struct INodeList
		{
			virtual INode* INodeList_GetNode(int i) = 0;
			virtual bool INodeList_IsNodeLastInBatch(int i) const = 0;
		};

		struct Worker
		{
			HANDLE mWorkAvailableEvent;
			HANDLE mWorkCompleteEvent;
			INodeList* mNodeList;
			HANDLE mThreadHandle;
			int mNodeIndexToBeProcessed;
			int mNumFrames;
		};

		INodeList* const mNodeList;
		Worker mWorkers[WaveSabreCore::kSongMaxThreads];

		GraphProcessor2(INodeList* nodeList) :
			mNodeList(nodeList)
		{
			for (int i = 0; i < WaveSabreCore::kSongMaxThreads; i++)
			{
				auto& worker = mWorkers[i];
				worker.mWorkAvailableEvent = CreateEvent(0, FALSE, FALSE, 0);
				worker.mWorkCompleteEvent = CreateEvent(0, FALSE, FALSE, 0);
				worker.mNodeList = nodeList;
				worker.mThreadHandle = CreateThread(0, 0, renderThreadProc, &worker, 0, 0);
				// on one hand this pulls in a DLL import, on the other hand a few bytes of text is trivial and this helps us get down to the precalc requirement.
				SetThreadPriority(worker.mThreadHandle, THREAD_PRIORITY_ABOVE_NORMAL);
			}
		}

		~GraphProcessor2()
		{
			// TODO: this.
			// currently, since this is not in the VST, we don't really care much as this is a fire-once kind of class.
			// ideally we would join all threads, close handles, close event handles ... but it will never be relevant.
#pragma message("GraphProcessor2::~GraphProcessor2() Leaking memory to save bits.")
		}

		void ProcessGraph(int numSamples)
		{
			// "frame" = 2 stereo samples.
			int numFrames = numSamples / 2;
			int currentBatchNodeIndex = 0; // index within the current batch.
			// right now, all threads should have
			// - mWorkAvailableEvent = not signalled
			// - mWorkCompleteEvent = not signalled
			for (int iExecOrder = 0; iExecOrder < WaveSabreCore::kSongTrackCount; iExecOrder++)
			{
				// assign this node to a worker.
				auto& worker = mWorkers[currentBatchNodeIndex];
				worker.mNodeIndexToBeProcessed = iExecOrder;
				worker.mNumFrames = numFrames;
				SetEvent(worker.mWorkAvailableEvent); // signal work is available.
				currentBatchNodeIndex ++; // assume enough threads (done by previous serialization)
				if (mNodeList->INodeList_IsNodeLastInBatch(iExecOrder))
				{
					// wait for the batch to complete. could use WaitForMultipleObjects, but
					// this does effectively the same thing, with fewer imports and less bookkeeping.
					for (int i = 0; i < currentBatchNodeIndex; i++)
					{
						auto& worker = mWorkers[i];
						WaitForSingleObject(worker.mWorkCompleteEvent, INFINITE);
						ResetEvent(worker.mWorkCompleteEvent);
					}
					currentBatchNodeIndex = 0;
				}
			}
		}

		static DWORD WINAPI renderThreadProc(LPVOID lpParameter)
		{
			Worker* worker = (Worker*)lpParameter;
			while (true)
			{
				WaitForSingleObject(worker->mWorkAvailableEvent, INFINITE);
				worker->mNodeList->INodeList_GetNode(worker->mNodeIndexToBeProcessed)->INode_Run(worker->mNumFrames);
				SetEvent(worker->mWorkCompleteEvent);
			}
			return 0;
		}

	}; // class GraphProcessor2

} // namespace WaveSabrePlayerLib

