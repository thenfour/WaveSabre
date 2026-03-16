
#include "VoiceAllocator.hpp"
#include "../Basic/Enum.hpp"

namespace WaveSabreCore
{
	void Voice::BaseNoteOn(const NoteInfo& ni, int unisonoVoice, VoiceNoteOnFlags flags)
	{
		mNoteInfo = ni;
		//mLegato = M7::HasFlag(flags, VoiceNoteOnFlags::Legato);
		mUnisonVoice = unisonoVoice;
		this->NoteOn(flags);
	}

	void Voice::BaseNoteOff() 
	{
		this->NoteOff();
	}
}
