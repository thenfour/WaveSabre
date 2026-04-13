// this is less of a synth device, and more of a synth voice manager.
// here we just deal with :
// - converting incoming raw MIDI events into more musically-relevant events, taking into consideration:
//   - sustain pedal
//   - unisono
// - distribution of events to voices.

#pragma once

#include "../Basic/Array.hpp"
#include "../Basic/CriticalSection.hpp"
#include "../Basic/DSPMath.hpp"
#include "../Basic/Helpers.h"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../WSCore/Device.h"


#include "VoiceAllocator.hpp"

namespace WaveSabreCore
{
class Maj7SynthDevice : public Device
{
public:
  Maj7SynthDevice(int numParams, float* paramCache);

  virtual void ProcessBlock(float* const* const outputs, int numSamples) = 0;

  virtual void Run(float** inputs, float** outputs, int numSamples) override;

  virtual void AllNotesOff();
  virtual void NoteOn(int note, int velocity, int deltaSamples);
  virtual void NoteOff(int note, int deltaSamples);

  void SetVoiceMode(VoiceMode voiceMode);

  void SetMaxVoices(int x)
  {
    AllNotesOff();  // helps make things predictable, reduce cases
    for (int i = 0; i < M7::gMaxMaxVoices; i++)
    {
      mVoices[i]->Kill(VoiceNoteOnFlags::Panic);
    }
    mMaxVoices = x;
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  int GetCurrentPolyphony();
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  enum class EventType  // : uint8_t
  {
    None,
    NoteOn,
    NoteOff,
    CC,
    PitchBend,
  };

  struct Event
  {
    int DeltaSamples;  // = 0;
    int data1;         // = 0;
    int data2;         // = 0;
    EventType Type;    // = EventType::None;
  };

  // always returns a voice. prefer stealing notes that are no longer musically held.
  M7::Pair<Voice*, VoiceNoteOnFlags> AllocateVoice()
  {
    Voice* oldestReleasedVoice = nullptr;
    Voice* oldestHeldVoice = nullptr;
    for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv)
    {
      auto* v = mVoices[iv];
      if (!v->IsPlaying())
        return { v, VoiceNoteOnFlags::None };

      Voice*& bestCandidate = v->mNoteInfo.mIsMusicallyHeld ? oldestHeldVoice : oldestReleasedVoice;
      if (!bestCandidate || (v->mNoteInfo.mSequence < bestCandidate->mNoteInfo.mSequence))
      {
        bestCandidate = v;
      }
    }
    Voice* ret = oldestReleasedVoice ? oldestReleasedVoice : oldestHeldVoice;
    return { ret, VoiceNoteOnFlags::VoiceSteal };
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
    if (mEventCount >= maxEvents)
    {
      // if you overflow event buffer, we don't have much choice but to abort mission. if you choose to ignore events,
      // you're likely going to ignore events that are musically continuous with previous events and will cause chaos.
      AllNotesOff();
    }
  }

  // finds the musically-held note with the highest sequence ID; this is the currently held monophonic note.
  // returns nullptr when no suitable note.
  NoteInfo* FindCurrentlyPlayingNote()
  {
    NoteInfo* pRet = nullptr;
    for (size_t i = 0; i < maxActiveNotes; ++i)
    {
      auto& x = mMonoNoteStates[i];
      if (!x.mIsMusicallyHeld)
        continue;
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
  void ProcessMusicalNoteOff(int note, NoteInfo& myNote);
  void ProcessNoteOffEvent(Event* e);

  void SetUnisonoVoices(int n)
  {
    AllNotesOff();  // helps make things predictable, reduce cases
    mVoicesUnisono = n;
  }

  // old max used to be 64. but in a DAW you can easily exceed this limit, for example fast moving chords + pitch bend / CC stuff
  // and DAW stalling like saving or other events that might interrupt processing a bit.
  // but note: this limit is only reached in DAWs, and rarely.
  static constexpr int maxEvents = 128;
  static constexpr int maxActiveNotes = 128;  // should always be 128 for all midi notes.

  // params, midi, other activity may come in on other threads. those threads need to be able to affect state concurrently
  // with ProcessEvents.
  // so, all data below this is protected by this critsec.
  CriticalSection mCritsec;

  int mMaxVoices = 32;
  int mVoicesUnisono = 1;  // # of voices to double.

  int mNoteSequence = 0;

  NoteInfo mMonoNoteStates[maxActiveNotes];  // index = midi note value; mono bookkeeping only

  M7::PodArray<Voice*, M7::gMaxMaxVoices>
      mVoices;  // allow child class to instantiate derived voice classes; don't template due to bloat.
  VoiceMode mVoiceMode = VoiceMode::Polyphonic;

  int mEventCount = 0;
  Event mEvents[maxEvents];
};
}  // namespace WaveSabreCore
