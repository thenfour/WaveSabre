#ifndef __WAVESABREPLAYERLIB_SONGRENDERER_H__
#define __WAVESABREPLAYERLIB_SONGRENDERER_H__

#include "CriticalSection.h"

#include <WaveSabreCore.h>

namespace WaveSabrePlayerLib
{
#ifdef MIN_SIZE_REL
	struct Stopwatch {
	};

	struct GraphRunnerProfiler
	{
		GraphRunnerProfiler(int nMaxParallel) {}

		void BeginGraph()
		{
		}
		void EndGraph()
		{
		}
		void BeginWork(Stopwatch& sw)
		{
		}
		void EndWork(Stopwatch& sw)
		{
		}
		void Dump()
		{
		}
	};

#else // #ifdef MIN_SIZE_REL
	class Stopwatch {
	public:
		Stopwatch() {
			LARGE_INTEGER f;
			QueryPerformanceFrequency(&f);
			m_frequency = f.QuadPart;
			m_ticksToMillisecondsDivisor = m_frequency / 1000;
		}

		void start() {
			if (m_running)
				return;
			m_startTick = QueryTicks();
			m_running = true;
		}

		void stop() {
			if (!m_running)
				return;
			auto currentTick = QueryTicks();
			m_elapsedTicks += currentTick - m_startTick;
			m_running = false;
		}

		void reset() {
			m_startTick = 0;
			m_elapsedTicks = 0;
			m_running = false;
		}

		uint64_t getElapsedTicks() const {
			uint64_t runningElapsedTicks = m_running ? (QueryTicks() - m_startTick) : 0;
			uint64_t r = m_elapsedTicks + runningElapsedTicks;
			return r;
		}

		uint32_t getElapsedMilliseconds() const {
			return (uint32_t)(getElapsedTicks() / m_ticksToMillisecondsDivisor);
		}

		void addTicks(uint64_t ms)
		{
			m_elapsedTicks += ms;
		}

	private:
		static uint64_t QueryTicks() {
			LARGE_INTEGER t;
			QueryPerformanceCounter(&t);
			return t.QuadPart;
		}

		uint64_t m_ticksToMillisecondsDivisor;
		bool m_running = false;
		uint64_t m_frequency = { 0 };
		uint64_t m_startTick = { 0 };
		std::atomic_uint64_t m_elapsedTicks = { 0 };
	};

	struct GraphRunnerProfiler
	{
		GraphRunnerProfiler(int nMaxParallel) :
			mNMaxParallel(nMaxParallel),
			mpParallelTimes(new Stopwatch[nMaxParallel + 1]) // 0 index is valid and accumulates idle time.
		{
		}
		~GraphRunnerProfiler()
		{
			delete[] mpParallelTimes;
		}

		const int mNMaxParallel;
		std::atomic_int mWorkersWorking = 0;
		Stopwatch mRuntime; // runtime spent in graph. no parallelism accounted for; this is what the user experiences.
		Stopwatch mWorktime; // amount of time spent doing work. because of parallelism this should be greater than runtime.
		Stopwatch* const mpParallelTimes; // runtime spent at various parallel levels. total should be roughly equal to mRuntime.
		std::atomic_bool mDump;

		void BeginGraph()
		{
			if (mDump) {
				mDump = false;
				char s[200];
				wsprintfA(s, "Runtime:  %u\r\n", mRuntime.getElapsedMilliseconds());
				::OutputDebugStringA(s);
				wsprintfA(s, "Worktime: %u\r\n", mWorktime.getElapsedMilliseconds());
				::OutputDebugStringA(s);
				auto wrr = MulDiv(mWorktime.getElapsedMilliseconds(), 100, mRuntime.getElapsedMilliseconds());
				wsprintfA(s, "Work/Run ratio: %d.%02d\r\n", (wrr / 100), (wrr % 100));
				::OutputDebugStringA(s);
				uint32_t maxParallelTime = 0;
				for (int i = 0; i < mNMaxParallel; ++i) {
					maxParallelTime = std::max(maxParallelTime, mpParallelTimes[i].getElapsedMilliseconds());
				}
				for (int i = 0; i < mNMaxParallel; ++i) {
					static constexpr size_t maxBarSize = 50;
					auto barCount = MulDiv(mpParallelTimes[i].getElapsedMilliseconds(), maxBarSize, maxParallelTime);
					char bar[maxBarSize * 2] = { 0 };
					memset(bar, '#', barCount);
					bar[barCount] = 0;
					wsprintfA(s, "Parallel time [%2d]: %s\r\n", i, bar);
					::OutputDebugStringA(s);
				}
			}
			mWorkersWorking = 0;
			mRuntime.start();
		}
		void EndGraph()
		{
			mRuntime.stop();
			mWorkersWorking = 0;
		}
		void BeginWork(Stopwatch& sw)
		{
			auto p = mWorkersWorking.fetch_add(1);
			mpParallelTimes[p].stop();
			mpParallelTimes[p + 1].start();
			sw.start();
		}
		void EndWork(Stopwatch& sw)
		{
			mWorktime.addTicks(sw.getElapsedTicks());
			auto p = mWorkersWorking.fetch_sub(1);
			mpParallelTimes[p].stop();
			mpParallelTimes[p - 1].start();
		}
		void Dump()
		{
			mDump = true;
		}
	};

#endif // #ifndef MIN_SIZE_REL

	extern GraphRunnerProfiler* gpGraphProfiler;

	class SongRenderer
	{
	public:
		enum class DeviceId
		{
			Falcon,
			Slaughter,
			Thunder,
			Scissor,
			Leveller,
			Crusher,
			Echo,
			Smasher,
			Chamber,
			Twister,
			Cathedral,
			Adultery,
			Specimen,
			Maj7,
			Maj7Width,
		};

		typedef WaveSabreCore::Device *(*DeviceFactory)(DeviceId);

		typedef struct {
			DeviceFactory factory;
			const unsigned char *blob;
		} Song;

		typedef short Sample;

		static const int NumChannels = 2;
		static const int BitsPerSample = 16;
		static const int BlockAlign = NumChannels * BitsPerSample / 8;

		SongRenderer(const SongRenderer::Song *song, int numRenderThreads);
		~SongRenderer();

		void RenderSamples(Sample *buffer, int numSamples);

		int GetTempo() const;
		int GetSampleRate() const;
		double GetLength() const;

	//private:
		enum class EventType
		{
			NoteOn = 0,
			NoteOff = 1,
			CC = 2,
			PitchBend = 3,
		};

		typedef struct
		{
			int TimeStamp;
			EventType Type;
			int Note, Velocity;
		} Event;

		class Devices
		{
		public:
			int numDevices;
			WaveSabreCore::Device **devices;
		};

		class MidiLane
		{
		public:
			int numEvents;
			Event *events;
		};

		class Track
		{
		public:
			typedef struct
			{
				int SendingTrackIndex;
				int ReceivingChannelIndex;
				float Volume;
			} Receive;

			Track(SongRenderer *songRenderer, DeviceFactory factory);
			~Track();
			
			void Run(int numSamples);

		private:
			static const int numBuffers = 4;
		public:
			float *Buffers[numBuffers];

			int NumReceives;
			Receive *Receives;

		private:
			class Automation
			{
			public:
				Automation(SongRenderer *songRenderer, WaveSabreCore::Device *device);
				~Automation();

				void Run(int numSamples);

			private:
				typedef struct
				{
					int TimeStamp;
					float Value;
				} Point;

				WaveSabreCore::Device *device;
				int paramId;

				int numPoints;
				Point *points;

				int samplePos;
				int pointIndex;
			};

			SongRenderer *songRenderer;

			float volume;

			int numDevices;
			int *devicesIndicies;

			int midiLaneId;

			int numAutomations;
			Automation **automations;

			int lastSamplePos;
			int accumEventTimestamp;
			int eventIndex;
		};

		enum class TrackRenderState : unsigned int
		{
			Idle,
			Rendering,
			Finished,
		};

		typedef struct
		{
			SongRenderer *songRenderer;
			int renderThreadIndex;
		} RenderThreadData;

		static DWORD WINAPI renderThreadProc(LPVOID lpParameter);

		bool renderThreadWork(int renderThreadIndex);

		// TODO: Templatize? Might actually be bigger..
		unsigned char readByte();
		int readInt();
		float readFloat();
		double readDouble();

		const unsigned char *songBlobPtr;
		int songDataIndex;

		int bpm;
		int sampleRate;
		double length;
	
		int numDevices;
		WaveSabreCore::Device **devices;

		int numMidiLanes;
		MidiLane **midiLanes;

		int numTracks;
		Track **tracks;
		TrackRenderState *trackRenderStates;

		int numRenderThreads;
		HANDLE *additionalRenderThreads;

		bool renderThreadShutdown;
		int renderThreadNumFloatSamples;
		unsigned int renderThreadsRunning;
		HANDLE *renderThreadStartEvents;
		HANDLE renderDoneEvent;
	};
}

#endif
