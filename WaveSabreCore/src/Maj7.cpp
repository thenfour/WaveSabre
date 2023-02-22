#include <WaveSabreCore/Maj7.hpp>
#include <WaveSabreCore/Helpers.h>

#include <math.h>

/*



3x osc1 enabled, volume, wave, shape, phaserest, phaseoff, porttime, sync enable, sync freq + kt, freq+kt, pitchsemis, pitchfine, freqmul, freqoff, filtermix
		   x               x               x                               x
3*(4+13*4) = 168 bytes

 1x amp env delaytime, attacktime, attackcurve, holdtime, decaytime, decaycurve, sustainlevel, releasetime, releasecurve, legatorestart
										 x                                x                                      x                  x
 2x mod env
3*(6*4+4) = 84 bytes

 2x lfo wave, shape, restart, phaseoff, freq
		   x           x
=28 bytes

 8x modulations enabled, source, destination, curve, scale
				  x         x         x        x
12*8=96 bytes
 pbrangesemis, mastervolume, unisono, detune, osc stereo spread, voicingmode
								 x                                   x
filter: type, q, saturation, frequency + kt
		  x
17 bytes
[9] FM matrix
36 bytes

=429 bytes = 7k per patch.
and i think we can order them in a way that helps compression. for example put curve values together. put all enableds together.




voicemgr






*/

namespace WaveSabreCore
{
	Maj7::Maj7()
		: Maj7SynthDevice((int)ParamIndices::NumParams)
	{
		for (auto& v : mVoices) {
			v = new Maj7Voice(this);
		}
	}

	void Maj7::SetParam(int index, float value)
	{
		switch ((ParamIndices)index)
		{
		case ParamIndices::Master:
			mMasterVolume = value;
			return;
		case ParamIndices::VoicingMode:
			SetVoiceMode(Helpers::ParamToVoiceMode(value));
			return;
		case ParamIndices::Unisono:
			SetUnisonoVoices(Helpers::ParamToUnisono(value));
			return;
		}
	}

	float Maj7::GetParam(int index) const
	{
		switch ((ParamIndices)index)
		{
		default:
		case ParamIndices::Master:
			return mMasterVolume;
		case ParamIndices::VoicingMode:
			return Helpers::VoiceModeToParam(GetVoiceMode());
		case ParamIndices::Unisono:
			return Helpers::UnisonoToParam(GetUnisonoVoices());
		}
	}

}
