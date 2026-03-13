
#include "VoiceAllocator.hpp"

namespace WaveSabreCore
{
	void Voice::BaseNoteOn(const NoteInfo& ni, int unisonoVoice, bool legato)
	{
		mNoteInfo = ni;
		mLegato = legato;
		mUnisonVoice = unisonoVoice;
		this->NoteOn();
	}

	void Voice::BaseNoteOff() 
	{
		this->NoteOff();
	}
}
