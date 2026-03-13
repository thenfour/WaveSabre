
#pragma once

#include "../Basic/Array.hpp"
#include "../Basic/DSPMath.hpp"

namespace WaveSabreCore
{

// zero-initialized POD for code space saving (yes it bloats)
struct NoteInfo
{
  int mSequence;      // = 0;
  int MidiNoteValue;  //= 0;
  int Velocity;  // = 0; // need to store all trigger info so when you release in mono triller we can re-trigger the note.
  bool mIsPhysicallyHeld;  // = false; // implies mIsMusicallyDown = true.
  bool mIsMusicallyHeld;   // = false;
};

struct Voice
{
  // child classes must implement
  //virtual void ProcessAndMix(double songPosition, float* const* const outputs, int numSamples, const float* oscDetuneSemis, const float* unisonoDetuneSemis) = 0;
  virtual void NoteOn() = 0;  // unisonoVoice is only valid for legato=false.
  virtual void NoteOff() = 0;
  virtual void
  Kill() = 0;  // forcibly (non-musically) kill the note, bypass envelopes. used for AllNotesOff or change of voice mode etc.
  virtual bool IsPlaying() = 0;  // subclass return true/false based on envelope / voice activity

  // called from Maj7SynthDevice
  void BaseNoteOn(const NoteInfo& ni, int unisonoVoice, bool legato);
  void BaseNoteOff();

  NoteInfo mNoteInfo;
  int mUnisonVoice = 0;
  bool mLegato;
};

// struct VoiceAllocator
// {
//     M7::PodArray<Voice*, M7::gMaxMaxVoices>& mVoices;
//     VoiceAllocator(M7::PodArray<Voice*, M7::gMaxMaxVoices>& voices) : mVoices(voices) {}
// };
}  // namespace WaveSabreCore