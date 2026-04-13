// why fork the other synth device?
// - i want to support sustain pedal
// - my synths have different behaviors regarding voicing and unisono
// - in order to make modifications i kinda had to refactor a bit to understand the flow better.

// 1. Midi events flow from Device into NoteOn, NoteOff, MidiCC, Pitchbend. these get added to a queue.
// 2. during buffer processing, these events are distributed to ProcessNoteOnEvent, ProcessNoteOffEvent, ProcessPedalEvent,  // HandleMidiCC, HandlePitchBend
// 3. there, we process pedal activity and pass to ProcessMusicalNoteOn, ProcessMusicalNoteOff
// 4. there, we distribute to voices based off voicing mode & unisono.

#include "Maj7SynthDevice.hpp"
#include "../Basic/Enum.hpp"

namespace WaveSabreCore
{
Maj7SynthDevice::Maj7SynthDevice(int numParams, float* paramCache)
    : Device(numParams, paramCache, nullptr)
{
  AllNotesOff();
}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
int Maj7SynthDevice::GetCurrentPolyphony()
{
  auto guard = this->mCritsec.Enter();
  int r = 0;
  for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv)
  {
    r += mVoices[iv]->IsPlaying() ? 1 : 0;
  }
  return r;
}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT


void Maj7SynthDevice::ProcessMusicalNoteOn(Event* __e, NoteInfo& myNote)
{
  myNote.mSequence = ++mNoteSequence;
  switch (mVoiceMode)
  {
    default:
    case VoiceMode::Polyphonic:
      myNote.mIsMusicallyHeld = true;
      for (int iuv = 0; iuv < mVoicesUnisono; ++iuv)
      {
        auto v = AllocateVoice();
        v.first->BaseNoteOn(myNote, iuv, v.second);
      }
      break;
    case VoiceMode::MonoLegatoTrill:
      NoteInfo* existingNote = FindCurrentlyPlayingNote();
      for (auto& noteState : mMonoNoteStates)
      {
        noteState.mIsMusicallyHeld = false;
      }
      myNote.mIsMusicallyHeld = true;

      for (int iuv = 0; iuv < mVoicesUnisono; ++iuv)
      {
        // steal the voice if it's already playing. that can happen even without "existingNote" in the case of
        // the voice being in release stage but not silent for example.
        auto* v = mVoices[iuv];
        bool isPlaying = v->IsPlaying();
        v->BaseNoteOn(myNote, iuv,
          M7::CombineFlags(
            M7::ConditionalFlag(isPlaying, VoiceNoteOnFlags::VoiceSteal),
            M7::ConditionalFlag(existingNote, VoiceNoteOnFlags::Legato)));
      }

      break;
  }
}

void Maj7SynthDevice::ProcessNoteOnEvent(Event* e)
{
  int note = e->data1;
  int velocity = e->data2;
  switch (mVoiceMode)
  {
    default:
    case VoiceMode::Polyphonic:
    {
      NoteInfo myNote = {};
      myNote.MidiNoteValue = note;
      myNote.Velocity = velocity;
      ProcessMusicalNoteOn(e, myNote);
      break;
    }
    case VoiceMode::MonoLegatoTrill:
      mMonoNoteStates[note].Velocity = velocity;
      ProcessMusicalNoteOn(e, mMonoNoteStates[note]);
      break;
  }
}

void Maj7SynthDevice::ProcessMusicalNoteOff(int note, NoteInfo& myNote)
{
  myNote.mIsMusicallyHeld = false;

  switch (mVoiceMode)
  {
    case VoiceMode::Polyphonic:
    default:
      for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv)
      {
        auto* v = mVoices[iv];
        if (v->mNoteInfo.MidiNoteValue != note)
          continue;
        if (!v->mNoteInfo.mIsMusicallyHeld)
          continue;
        v->mNoteInfo.mIsMusicallyHeld = false;
        v->BaseNoteOff();
      }
      break;
    case VoiceMode::MonoLegatoTrill:
      for (int iuv = 0; iuv < mVoicesUnisono; ++iuv)
      {
        mVoices[iuv]->mNoteInfo.mIsMusicallyHeld = false;
        mVoices[iuv]->BaseNoteOff();
      }
      break;
  }
}

void Maj7SynthDevice::ProcessNoteOffEvent(Event* e)
{
  int note = e->data1;
  if (mVoiceMode == VoiceMode::Polyphonic)
  {
    NoteInfo myNote = {};
    myNote.MidiNoteValue = note;
    ProcessMusicalNoteOff(note, myNote);
    return;
  }

  NoteInfo& ni = mMonoNoteStates[note];
  if (!ni.mIsMusicallyHeld)
  {
    return;
  }

  ProcessMusicalNoteOff(note, ni);
}

// plays through the buffer,
// handling events and processing audio in sub-chunks broken up by events.
void Maj7SynthDevice::Run(float** inputs, float** __outputs, int numSamples)
{
  clearOutputs(__outputs, numSamples);

  float* runningOutputs[2] = {__outputs[0],
                              __outputs[1]};  // make a copy because we'll be advancing these cursors ourselves.

  // assuming that the DAW always gives us events in chronological order lets us optimize a bit.
  // it means we can short circuit the moment we see an event in the future and not bother looking for other matching events.
  // it also means we can just cursor along events in sequence each sub-chunk.
  // it also means we can just slide over the 1 block of obsolete events each main chunk, rather than hunting & pecking to defragment. we assume there are no fragments.
  int iEvent = 0;

  auto guard = this->mCritsec.Enter();

  while (numSamples)
  {
    int samplesToNextEvent =
        numSamples;  // initially assume we'll process the remainder of the buffer. this may change if we hit an event to mark the next sub-chunk.
    while (iEvent < mEventCount)
    {
      Event* e = &mEvents[iEvent];
      if (e->Type == EventType::None)
      {
        ++iEvent;
        continue;
      }

      if (e->DeltaSamples > 0)
      {
        if (e->DeltaSamples < samplesToNextEvent)
        {
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
          ProcessNoteOnEvent(e);
          break;
        case EventType::NoteOff:
          ProcessNoteOffEvent(e);
          break;
      }

      mEvents[iEvent].Type = EventType::None;
      ++iEvent;
    }

    this->ProcessBlock(runningOutputs, samplesToNextEvent);

    // advance a virtual cursor
    for (int i = iEvent; i < mEventCount; i++)
    {
      mEvents[i].DeltaSamples -= samplesToNextEvent;
    }
    //songPosition += (double)samplesToNextEvent / Helpers::CurrentSampleRate;
    runningOutputs[0] += samplesToNextEvent;
    runningOutputs[1] += samplesToNextEvent;
    numSamples -= samplesToNextEvent;
  }

  // fix event count now that we have flushed some. so remove gaps and recalculate the count.
  // iEvent points to the "end" of processed events.
  memmove(&mEvents[0], &mEvents[iEvent], (mEventCount - iEvent) * sizeof(Event));

  mEventCount -= iEvent;
}

void Maj7SynthDevice::AllNotesOff()
{
  auto guard = this->mCritsec.Enter();
  for (int i = 0; i < M7::gMaxMaxVoices; i++)
  {
    if (!mVoices[i])
      continue;  // necessary because on ctor this gets called before voices are cerated. because this function is an initialization of state as well as all notes off.
    mVoices[i]->Kill(VoiceNoteOnFlags::Panic);
  }
  // clear events
  mEventCount = 0;
  memset(mEvents, 0, sizeof(mEvents[0]) * maxEvents);

  memset(mMonoNoteStates, 0, sizeof(mMonoNoteStates[0]) * maxActiveNotes);
  for (int i = 0; i < maxActiveNotes; i++)
  {
    mMonoNoteStates[i].MidiNoteValue = i;
  }
}

void Maj7SynthDevice::NoteOn(int note, int velocity, int deltaSamples)
{
  if (velocity == 0)
  {
    NoteOff(note, deltaSamples);
  }
  else
  {
    PushEvent(EventType::NoteOn, note, velocity, deltaSamples);
  }
}

void Maj7SynthDevice::NoteOff(int note, int deltaSamples)
{
  PushEvent(EventType::NoteOff, note, 0, deltaSamples);
}

void Maj7SynthDevice::SetVoiceMode(VoiceMode voiceMode)
{
  auto guard = this->mCritsec.Enter();
  AllNotesOff();
  for (int i = 0; i < M7::gMaxMaxVoices; i++)
  {
    mVoices[i]->Kill(VoiceNoteOnFlags::Panic);
  }
  this->mVoiceMode = voiceMode;
}

}  // namespace WaveSabreCore
