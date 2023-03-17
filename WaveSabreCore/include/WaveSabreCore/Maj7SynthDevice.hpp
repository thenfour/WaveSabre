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

namespace WaveSabreCore
{
	class Maj7SynthDevice : public Device
	{
	public:
		Maj7SynthDevice(int numParams);

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
		VoiceMode GetVoiceMode() const;

		// zero-initialized POD for code space saving (yes it bloats)
		struct NoteInfo
		{
			bool mIsPhysicallyHeld;// = false; // implies mIsMusicallyDown = true.
			bool mIsMusicallyHeld;// = false;
			int mSequence;// = 0;
			int MidiNoteValue;//= 0;
			int Velocity;// = 0; // need to store all trigger info so when you release in mono triller we can re-trigger the note.
			//void Clear() {
			//	mIsPhysicallyHeld = false;
			//	mIsMusicallyHeld = false;
			//}
		};

		int GetCurrentPolyphony() const {
			int r = 0;
			for (auto& v : mVoices) {
				r += v->IsPlaying() ? 1 : 0;
			}
			return r;
		}

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

		enum class EventType
		{
			None,
			NoteOn,
			NoteOff,
			CC,
			PitchBend,
		};

		struct Event
		{
			EventType Type;// = EventType::None;
			int DeltaSamples;// = 0;
			int data1;// = 0;
			int data2;// = 0;
		};

		void PushEvent(EventType et, int data1, int data2, int deltaSamples)
		{
			auto& e = mEvents[mEventCount];
			e.Type = et;
			e.data1 = data1;
			e.data2 = data2;
			e.DeltaSamples = deltaSamples;
			// this assumes events are chronological and that we'll never get events during a pruning operation.

			mEventCount++;
		}

		void clearEvents()
		{
			memset(mEvents, 0, sizeof(mEvents[0]) * maxEvents);
			mEventCount = 0;
		}

		// always returns a voice. ideally we would look at envelope states to determine the most suitable, but let's just keep it simple and increase max poly
		Voice* FindFreeVoice() {
			Voice* playingVoiceToReturn = mVoices[0];
			for (auto* v : mVoices) {
				if (!v->IsPlaying())
					return v;
				if (!playingVoiceToReturn || (v->mNoteInfo.mSequence < playingVoiceToReturn->mNoteInfo.mSequence))
				{
					playingVoiceToReturn = v;
				}
			}
			return playingVoiceToReturn;
		}

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
			for (size_t i = 0; i < maxActiveNotes; ++i)
			{
				auto& x = mNoteStates[i];
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

		void ProcessMusicalNoteOn(Event* e, NoteInfo& myNote) {
			switch (mVoiceMode)
			{
			default:
			case VoiceMode::Polyphonic:
				myNote.mSequence = ++mNoteSequence;

				if (myNote.mIsMusicallyHeld)
				{
					// this is already playing, in the case you have sustain pedal down.
					// re-send a note on to the existing.
					for (auto* pv : mVoices)
					{
						if (pv->mNoteInfo.MidiNoteValue != myNote.MidiNoteValue)
							continue;
						pv->BaseNoteOn(myNote, pv->mUnisonVoice, true /* i mean, is it though? */);
					}
				}
				else {
					myNote.mIsMusicallyHeld = true;
					for (int iuv = 0; iuv < mVoicesUnisono; ++iuv)
					{
						auto v = FindFreeVoice();
						v->BaseNoteOn(myNote, iuv, false);
					}
				}

				break;
			case VoiceMode::MonoLegatoTrill:
				// this assumes that the trill note is the one currently playing, if exists.
				NoteInfo* existingNote = FindTrillNote(myNote.MidiNoteValue);
				if (existingNote) {
					existingNote->mIsMusicallyHeld = false;
				}

				// important that this comes after looking for existing note, otherwise it will think THIS note is the trill note.
				myNote.mIsMusicallyHeld = true;
				myNote.mSequence = ++mNoteSequence;

				for (int iuv = 0; iuv < mVoicesUnisono; ++iuv)
				{
					mVoices[iuv]->BaseNoteOn(myNote, iuv, !!existingNote);
				}

				break;
			}
		}

		void ProcessNoteOnEvent(Event* e)
		{
			int note = e->data1;
			mNoteStates[note].mIsPhysicallyHeld = true;
			mNoteStates[note].Velocity = e->data2;

			ProcessMusicalNoteOn(e, mNoteStates[note]);
		}

		void ProcessMusicalNoteOff(int note, NoteInfo& myNote, NoteInfo* pPlaying) {
			switch (mVoiceMode)
			{
			case VoiceMode::Polyphonic:
			default:
				myNote.mIsMusicallyHeld = false;
				for (auto* v : mVoices)
				{
					if (v->mNoteInfo.MidiNoteValue == note) {
						v->NoteOff();
					}
				}
				break;
			case VoiceMode::MonoLegatoTrill:

				myNote.mIsMusicallyHeld = false;

				auto pTrillNote = FindTrillNote(myNote.MidiNoteValue);
				if (pTrillNote) {
					if (pPlaying && pPlaying->MidiNoteValue != myNote.MidiNoteValue) {
						// if the note being lifted is not the one playing, don't do anything;
						// it doesn't trigger another note on, and it's not playing so shouldn't get a note off.
						return;
					}
					pTrillNote->mSequence = ++mNoteSequence;
					pTrillNote->mIsMusicallyHeld = true;
				}

				for (int iuv = 0; iuv < mVoicesUnisono; ++iuv) // weird structuring here for size optimization
				{
					if (pTrillNote)
					{
						mVoices[iuv]->BaseNoteOn(*pTrillNote, iuv, true);
					}
					else {
						mVoices[iuv]->BaseNoteOff();
					}
				}
				break;
			}
		}

		// process physical state, convert to musical state.
		void ProcessNoteOffEvent(Event* e) {
			int note = e->data1;
			NoteInfo& ni = mNoteStates[note];
			if (!ni.mIsPhysicallyHeld) {
				return;
			}

			//auto pPlaying = FindTrillNote(-1); // returns the currently playing note (useful for monophonic processing)
			auto pPlaying = FindCurrentlyPlayingNote();

			ni.mIsPhysicallyHeld = false;
			if (this->mIsPedalDown) {
				return;// don't affect musical state; nothing more to do.
			}
			// pedal is up
			ProcessMusicalNoteOff(note, ni, pPlaying);
		}

		void ProcessPedalEvent(Event* e, bool isDown)
		{
			if (isDown) {
				mIsPedalDown = true;
				return;
			}
			if (isDown == mIsPedalDown) {
				return;
			}

			// returns the currently playing note (useful for monophonic processing)
			auto pPlaying = FindCurrentlyPlayingNote();

			// handle pedal up.
			mIsPedalDown = false;

			for (size_t i = 0; i < maxActiveNotes; ++i)
			{
				auto& x = mNoteStates[i];
				if (x.mIsPhysicallyHeld) {
					continue;
				}
				ProcessMusicalNoteOff(x.MidiNoteValue, x, pPlaying);
			}
		}

		void SetUnisonoVoices(int n) {
			AllNotesOff(); // helps make things predictable, reduce cases
			mVoicesUnisono = n;
		}
		int GetUnisonoVoices()const {
			return mVoicesUnisono;
		}

		static constexpr int maxVoices = 32;// for the sake of CPU savings don't go crazy. if this is too high then long release times or big unisono will just kill a CPU.
		static constexpr int maxEvents = 64;
		static constexpr int maxActiveNotes = 128; // should always be 128 for all midi notes.

		int mVoicesUnisono = 1; // # of voices to double.
		bool mIsPedalDown = false;

		int mNoteSequence = 0;

		NoteInfo mNoteStates[maxActiveNotes] ; // index = midi note value

		Voice* mVoices[maxVoices] = { 0 }; // allow child class to instantiate derived voice classes; don't template due to bloat.
		VoiceMode mVoiceMode = VoiceMode::Polyphonic;

		Event mEvents[maxEvents];
		int mEventCount = 0;
	};
}

