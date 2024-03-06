// why fork the other synth device?
// - i want to support sustain pedal
// - my synths have different behaviors regarding voicing and unisono
// - in order to make modifications i kinda had to refactor a bit to understand the flow better.

// 1. Midi events flow from Device into NoteOn, NoteOff, MidiCC, Pitchbend. these get added to a queue.
// 2. during buffer processing, these events are distributed to ProcessNoteOnEvent, ProcessNoteOffEvent, ProcessPedalEvent,  // HandleMidiCC, HandlePitchBend
// 3. there, we process pedal activity and pass to ProcessMusicalNoteOn, ProcessMusicalNoteOff
// 4. there, we distribute to voices based off voicing mode & unisono.

#include <WaveSabreCore.h>
#include <WaveSabreCore/Maj7SynthDevice.hpp>
#include <WaveSabreCore/Helpers.h>
//#include <Windows.h>
#include <cstdint>
#include <cstdio>

namespace WaveSabreCore
{
	Maj7SynthDevice::Maj7SynthDevice(int numParams, float* paramCache)
		: Device(numParams, paramCache, nullptr)
	{
		AllNotesOff();
	}

	int Maj7SynthDevice::GetCurrentPolyphony() const {
		int r = 0;
		for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv) {
			r += mVoices[iv]->IsPlaying() ? 1 : 0;
		}
		return r;
	}



	void Maj7SynthDevice::ProcessMusicalNoteOn(Event* e, NoteInfo& myNote) {
		switch (mVoiceMode)
		{
		default:
		case VoiceMode::Polyphonic:
			myNote.mSequence = ++mNoteSequence;

			if (myNote.mIsMusicallyHeld)
			{
				// this is already playing, in the case you have sustain pedal down.
				// re-send a note on to the existing.
				//for (auto* pv : mVoices)
				for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv)
				{
					auto* pv = mVoices[iv];
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

	void Maj7SynthDevice::ProcessNoteOnEvent(Event* e)
	{
		int note = e->data1;
		int velocity = e->data2;
		mNoteStates[note].mIsPhysicallyHeld = true;
		mNoteStates[note].Velocity = e->data2;
		ProcessMusicalNoteOn(e, mNoteStates[note]);
	}

	void Maj7SynthDevice::ProcessMusicalNoteOff(int note, NoteInfo& myNote, NoteInfo* pPlaying)
	{

		myNote.mIsMusicallyHeld = false;

		switch (mVoiceMode)
		{
		case VoiceMode::Polyphonic:
		default:
			for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv)
			{
				auto* v = mVoices[iv];
				if (v->mNoteInfo.MidiNoteValue == note) {
					v->NoteOff();
				}
			}
			break;
		case VoiceMode::MonoLegatoTrill:

			//myNote.mIsMusicallyHeld = false;

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
	void Maj7SynthDevice::ProcessNoteOffEvent(Event* e) {
		int note = e->data1;
		NoteInfo& ni = mNoteStates[note];
		if (!ni.mIsPhysicallyHeld) {
			return;
		}

		auto pPlaying = FindCurrentlyPlayingNote();

		ni.mIsPhysicallyHeld = false;
		if (this->mIsPedalDown) {
			return;// don't affect musical state; nothing more to do.
		}
		// pedal is up
		ProcessMusicalNoteOff(note, ni, pPlaying);
	}

	void Maj7SynthDevice::ProcessPedalEvent(Event* e, bool isDown)
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



	// plays through the buffer,
	// handling events and processing audio in sub-chunks broken up by events.
	void Maj7SynthDevice::Run(double songPosition, float **inputs, float **__outputs, int numSamples)
	{
		clearOutputs(__outputs, numSamples);

		float *runningOutputs[2] = { __outputs[0], __outputs[1]}; // make a copy because we'll be advancing these cursors ourselves.

		// assuming that the DAW always gives us events in chronological order lets us optimize a bit.
		// it means we can short circuit the moment we see an event in the future and not bother looking for other matching events.
		// it also means we can just cursor along events in sequence each sub-chunk.
		// it also means we can just slide over the 1 block of obsolete events each main chunk, rather than hunting & pecking to defragment. we assume there are no fragments.
		int iEvent = 0;

		while (numSamples)
		{
			int samplesToNextEvent = numSamples; // initially assume we'll process the remainder of the buffer. this may change if we hit an event to mark the next sub-chunk.
			while (iEvent < mEventCount)
			{
				Event* e = &mEvents[iEvent];
				if (e->Type == EventType::None)
				{
					++ iEvent;
					continue;
				}
				
				if (e->DeltaSamples > 0)
				{
					if (e->DeltaSamples < samplesToNextEvent) {
						// we reached an event in the future but within this buffer. it implies that we finished processing immediate events; move to the next subchunk.
						samplesToNextEvent = e->DeltaSamples;
					}
					// else: event will happen in a future buffer; just break and let the buffer play out.
					break;
				}

				// event should be processed.
				switch (e->Type)
				{
				case EventType::NoteOn:
					//cc::log("[buf:%d] Handling note on event; note=%d, velocity=%d, deltasamples=%d", cc::gBufferCount, e->data1, e->data2, e->DeltaSamples);
					ProcessNoteOnEvent(e);
					break;
				case EventType::NoteOff:
					//cc::log("[buf:%d] Handling note off event; note=%d, deltasamples=%d", cc::gBufferCount, e->data1, e->DeltaSamples);
					ProcessNoteOffEvent(e);
					break;
				case EventType::CC:
					if (e->data1 == 64) {
						// handle pedal down/up
						ProcessPedalEvent(e, e->data2 >= 64);
					}
					HandleMidiCC(e->data1, e->data2);
					break;
				case EventType::PitchBend:
					int bend14 = (e->data2 << 7) | e->data1; // combine two 7-bit fields into 14-bit
					bend14 -= 8192;
					float normalized_bend = (float(bend14) / 8192); // bend controller in -1.0 to +1.0 range
					//float normalized_bend = (float(bend14) / 8192.0f) - 1.0f; // bend controller in -1.0 to +1.0 range
					HandlePitchBend(normalized_bend);
					break;
				}

				mEvents[iEvent].Type = EventType::None;
				++iEvent;
			}

			this->ProcessBlock(songPosition, runningOutputs, samplesToNextEvent);

			// advance a virtual cursor
			for (int i = iEvent; i < mEventCount; i++)
			{
				mEvents[i].DeltaSamples -= samplesToNextEvent;
			}
			songPosition += (double)samplesToNextEvent / Helpers::CurrentSampleRate;
			runningOutputs[0] += samplesToNextEvent;
			runningOutputs[1] += samplesToNextEvent;
			numSamples -= samplesToNextEvent;
		}

		// fix event count now that we have flushed some. so remove gaps and recalculate the count.
		// iEvent points to the "end" of processed events.
		memmove(&mEvents[0], &mEvents[iEvent], (mEventCount - iEvent) * sizeof(Event));
		//for (int i = iEvent; i < mEventCount; ++i) {
		//	mEvents[i - iEvent] = mEvents[i];
		//}

		mEventCount -= iEvent;
	}

	void Maj7SynthDevice::AllNotesOff()
	{
		for (int i = 0; i < M7::gMaxMaxVoices; i++)
		{
			if (!mVoices[i]) continue; // necessary because on ctor this gets called before voices are cerated. because this function is an initialization of state as well as all notes off.
			mVoices[i]->NoteOff();
		}
		// clear events
		mEventCount = 0;
		memset(mEvents, 0, sizeof(mEvents[0]) * maxEvents);

		memset(mNoteStates, 0, sizeof(mNoteStates[0]) * maxActiveNotes);
		for (int i = 0; i < maxActiveNotes; i++)
		{
			mNoteStates[i].MidiNoteValue = i;
		}

	}

	void Maj7SynthDevice::NoteOn(int note, int velocity, int deltaSamples)
	{
		//cc::log("[buf:%d] Pushing note on event; note=%d, velocity=%d, deltasamples=%d", cc::gBufferCount, note, velocity, deltaSamples);
		if (velocity == 0) {
			//PushEvent(EventType::NoteOff, note, 0, deltaSamples);
			NoteOff(note, deltaSamples);
		}
		else {
			PushEvent(EventType::NoteOn, note, velocity, deltaSamples);
		}
	}

	void Maj7SynthDevice::NoteOff(int note, int deltaSamples)
	{
		//cc::log("[buf:%d] Pushing note off event; note=%d, deltasamples=%d", cc::gBufferCount, note, deltaSamples);
		PushEvent(EventType::NoteOff, note, 0, deltaSamples);
	}

	void Maj7SynthDevice::MidiCC(int ccNumber, int value, int deltaSamples)
	{
		PushEvent(EventType::CC, ccNumber, value, deltaSamples);
	}
	void Maj7SynthDevice::PitchBend(int lsb, int msb, int deltaSamples)
	{
		PushEvent(EventType::PitchBend, lsb, msb, deltaSamples);
	}

	void Maj7SynthDevice::SetVoiceMode(VoiceMode voiceMode)
	{
		AllNotesOff();
		for (int i = 0; i < M7::gMaxMaxVoices; i++)
		{
			mVoices[i]->Kill();
		}
		this->mVoiceMode = voiceMode;
	}

	VoiceMode Maj7SynthDevice::GetVoiceMode() const
	{
		return mVoiceMode;
	}






	void Maj7SynthDevice::Voice::BaseNoteOn(const NoteInfo& ni, int unisonoVoice, bool legato)
	{
		mNoteInfo = ni;
		mLegato = legato;
		mUnisonVoice = unisonoVoice;
		this->NoteOn();
	}

	void Maj7SynthDevice::Voice::BaseNoteOff() 
	{
		this->NoteOff();
	}






}
