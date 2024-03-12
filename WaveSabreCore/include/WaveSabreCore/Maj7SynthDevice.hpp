// this is less of a synth device, and more of a synth voice manager.
// here we just deal with :
// - converting incoming raw MIDI events into more musically-relevant events, taking into consideration:
//   - sustain pedal
//   - unisono
// - distribution of events to voices.

#pragma once

#include <WaveSabreCore/Helpers.h>
#include "SynthDevice.h"
#include <algorithm>
#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{
	class Maj7SynthDevice : public Device
	{
	public:
		Maj7SynthDevice(int numParams, float *paramCache);

		virtual void ProcessBlock(double songPosition, float* const* const outputs, int numSamples) = 0;
		virtual void HandlePitchBend(float pbN11) = 0;
		virtual void HandleMidiCC(int ccN, int val) = 0;

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples);

		virtual void AllNotesOff();
		virtual void NoteOn(int note, int velocity, int deltaSamples);
		virtual void NoteOff(int note, int deltaSamples);
		virtual void MidiCC(int ccNumber, int value, int deltaSamples) override;
		virtual void PitchBend(int lsb, int msb, int deltaSamples) override;

		void SetVoiceMode(VoiceMode voiceMode);
		//VoiceMode GetVoiceMode() const;

		void SetMaxVoices(int x)
		{
			AllNotesOff(); // helps make things predictable, reduce cases
			for (int i = 0; i < M7::gMaxMaxVoices; i++)
			{
				mVoices[i]->Kill();
			}
			mMaxVoices = x;
		}

		// zero-initialized POD for code space saving (yes it bloats)
		struct NoteInfo
		{
			int mSequence;// = 0;
			int MidiNoteValue;//= 0;
			int Velocity;// = 0; // need to store all trigger info so when you release in mono triller we can re-trigger the note.
			bool mIsPhysicallyHeld;// = false; // implies mIsMusicallyDown = true.
			bool mIsMusicallyHeld;// = false;
		};

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		int GetCurrentPolyphony();
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		struct Voice
		{
			// child classes must implement
			//virtual void ProcessAndMix(double songPosition, float* const* const outputs, int numSamples, const float* oscDetuneSemis, const float* unisonoDetuneSemis) = 0;
			virtual void NoteOn() = 0; // unisonoVoice is only valid for legato=false.
			virtual void NoteOff() = 0;
			virtual void Kill() = 0; // forcibly (non-musically) kill the note, bypass envelopes. used for AllNotesOff or change of voice mode etc.
			virtual bool IsPlaying() = 0; // subclass return true/false based on envelope / voice activity

			// called from Maj7SynthDevice
			void BaseNoteOn(const NoteInfo& ni, int unisonoVoice, bool legato);
			void BaseNoteOff();

			NoteInfo mNoteInfo;
			int mUnisonVoice = 0;
			bool mLegato;
		};

		enum class EventType// : uint8_t
		{
			None,
			NoteOn,
			NoteOff,
			CC,
			PitchBend,
		};

		struct Event
		{
			int DeltaSamples;// = 0;
			int data1;// = 0;
			int data2;// = 0;
			EventType Type;// = EventType::None;
		};

	// always returns a voice. ideally we would look at envelope states to determine the most suitable, but let's just keep it simple and increase max poly
		Voice* FindFreeVoice() {
			Voice* playingVoiceToReturn = mVoices[0];
			for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv) {
				auto* v = mVoices[iv];
				if (!v->IsPlaying())
					return v;
				if (!playingVoiceToReturn || (v->mNoteInfo.mSequence < playingVoiceToReturn->mNoteInfo.mSequence))
				{
					playingVoiceToReturn = v;
				}
			}
			return playingVoiceToReturn;
		}



		void PushEvent(EventType et, int data1, int data2, int deltaSamples)
		{
			auto& e = mEvents[mEventCount];
			e.Type = et;
			e.data1 = data1;
			e.data2 = data2;
			e.DeltaSamples = deltaSamples;
			// this assumes events are chronological and that we'll never get events during a pruning operation.

			mEventCount++;
			if (mEventCount >= maxEvents) {
				// if you overflow event buffer, we don't have much choice but to abort mission. if you choose to ignore events,
				// you're likely going to ignore events that are musically continuous with previous events and will cause chaos.
				AllNotesOff();
			}
		}

		// do not extern; inline is best.
		// finds the physically-held note with the highest sequence ID, which can be used as a trill note in monophonic mode.
		// returns nullptr when no suitable note.
		NoteInfo* FindTrillNote(int ignoreMidiNote)
		{
			NoteInfo* pTrill = nullptr;
			// figure out the last physically-held note, in order to do trilling monophonic behavior.
			// this flow happens when you for example,
			// 1. hold note A
			// 2. pedal down
			// 3. hold note B, release note B. pedal is currently down so you hear note B.
			// 4. release pedal. note A is still held so it should now be playing.
			for (auto& x : mNoteStates)
			{
				if (x.mIsPhysicallyHeld && (x.MidiNoteValue != ignoreMidiNote)) {
					if (pTrill && x.mSequence < pTrill->mSequence)
						continue; // we're looking for the latest.
					pTrill = &x;
				}
			}
			return pTrill;
		}

		// finds the musically-held note with the highest sequence ID; this is the currently playing monophonic note.
		// returns nullptr when no suitable note.
		NoteInfo* FindCurrentlyPlayingNote()
		{
			NoteInfo* pRet = nullptr;
			for (size_t i = 0; i < maxActiveNotes; ++i)
			{
				auto& x = mNoteStates[i];
				if (!x.mIsMusicallyHeld) continue;
				if (pRet && x.mSequence < pRet->mSequence)
				{
					continue;
				}
				pRet = &x;
			}
			return pRet;
		}




		void ProcessMusicalNoteOn(Event* e, NoteInfo& myNote);

		void ProcessNoteOnEvent(Event* e);
		void ProcessMusicalNoteOff(int note, NoteInfo& myNote, NoteInfo* pPlaying);
		// process physical state, convert to musical state.
		void ProcessNoteOffEvent(Event* e);

		void ProcessPedalEvent(Event* e, bool isDown);

		void SetUnisonoVoices(int n) {
			AllNotesOff(); // helps make things predictable, reduce cases
			mVoicesUnisono = n;
		}
		//int GetUnisonoVoices()const {
		//	return mVoicesUnisono;
		//}

		// old max used to be 64. but in a DAW you can easily exceed this limit, for example fast moving chords + pitch bend / CC stuff
		// and DAW stalling like saving or other events that might interrupt processing a bit.
		// but note: this limit is only reached in DAWs, and rarely.
		static constexpr int maxEvents = 128;
		static constexpr int maxActiveNotes = 128; // should always be 128 for all midi notes.

		// params, midi, other activity may come in on other threads. those threads need to be able to affect state concurrently
		// with ProcessEvents.
		// so, all data below this is protected by this critsec.
		CriticalSection mCritsec;

		int mMaxVoices = 32;
		int mVoicesUnisono = 1; // # of voices to double.

		int mNoteSequence = 0;

		NoteInfo mNoteStates[maxActiveNotes] ; // index = midi note value

		Voice* mVoices[M7::gMaxMaxVoices] = { 0 }; // allow child class to instantiate derived voice classes; don't template due to bloat.
		VoiceMode mVoiceMode = VoiceMode::Polyphonic;

		int mEventCount = 0;
		Event mEvents[maxEvents];
		bool mIsPedalDown = false;
	};
}

