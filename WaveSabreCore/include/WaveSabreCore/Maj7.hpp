#pragma once

#include <WaveSabreCore/Helpers.h>
#include "Maj7SynthDevice.hpp"
#include <Windows.h>

namespace WaveSabreCore
{
	class Maj7 : public Maj7SynthDevice
	{
	public:
		enum class ParamIndices
		{
			Master,
			VoicingMode,
			Unisono,
			NumParams,
		};

		Maj7();

		virtual void HandlePitchBend(float pbN11) {
			cc::log("Maj7::HandlePitchBend %.3f", pbN11);
		}
		virtual void HandleMidiCC(int ccN, int val) {
			cc::log("Maj7::HandleMidiCC %02x = %02x", ccN, val);

		}
		virtual void SetParam(int index, float value) override;
		virtual float GetParam(int index) const override;

		float mMasterVolume = 1.0f;

		float mLatestFreq = 0;
		int mLatestNote = 0;

		struct Maj7Voice : public Maj7SynthDevice::Voice
		{
			Maj7Voice(Maj7* owner) : mpOwner(owner) {
			}

			Maj7* mpOwner;
			bool mRunning = false;
			float phase = 0;

			virtual void ProcessAndMix(double songPosition, float* const* const  outputs, int numSamples) override
			{
				if (!mRunning) return;
				for (int iSample = 0; iSample < numSamples; ++iSample) {
					auto freq = (float)Helpers::NoteToFreq(mNoteInfo.MidiNoteValue);
					mpOwner->mLatestFreq = freq;
					mpOwner->mLatestNote = mNoteInfo.MidiNoteValue;
					auto dt = freq / Helpers::CurrentSampleRate;// 44100.0f;
					phase += (float)dt;
					phase = phase - floorf(phase);

					float s = ::sinf(phase * 2 * 3.14159) * mpOwner->mMasterVolume * (mNoteInfo.Velocity / 128.0);

					outputs[0][iSample] += s;
					outputs[1][iSample] += s;
				}
			}
			virtual void NoteOn() override {
				mRunning = true;
			}
			virtual void NoteOff() override {
				mRunning = false;
			}
			virtual void Kill() override {
				mRunning = false;
			}
			virtual bool IsPlaying() override {
				return mRunning;
			}
		};


	};
}

