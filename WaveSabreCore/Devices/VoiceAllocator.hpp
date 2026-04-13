
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
  //bool mIsPhysicallyHeld;  // = false; // implies mIsMusicallyDown = true.
  bool mIsMusicallyHeld;   // = false;
};

enum class VoiceNoteOnFlags
{
  None = 0,
  Legato = 1 << 0,
  VoiceSteal = 1 << 1,  // signal to the voice to handle transition properly. #155
  Panic = 1 << 2,
};

struct Voice
{
  // child classes must implement
  //virtual void ProcessAndMix(double songPosition, float* const* const outputs, int numSamples, const float* oscDetuneSemis, const float* unisonoDetuneSemis) = 0;
  virtual void NoteOn(VoiceNoteOnFlags flags) = 0;  // unisonoVoice is only valid for legato=false.
  virtual void NoteOff() = 0;
  // forcibly (non-musically) kill the note, bypass envelopes. used for AllNotesOff or change of voice mode etc.
  // 
  virtual void Kill(VoiceNoteOnFlags flags) = 0;  
  virtual bool IsPlaying() = 0;  // subclass return true/false based on envelope / voice activity

  // called from Maj7SynthDevice
  void BaseNoteOn(const NoteInfo& ni, int unisonoVoice, VoiceNoteOnFlags flags);
  void BaseNoteOff();

  NoteInfo mNoteInfo;
  int mUnisonVoice = 0;
  //bool mLegato;
};

// struct VoiceAllocator
// {
//     M7::PodArray<Voice*, M7::gMaxMaxVoices>& mVoices;
//     VoiceAllocator(M7::PodArray<Voice*, M7::gMaxMaxVoices>& voices) : mVoices(voices) {}
// };
}  // namespace WaveSabreCore