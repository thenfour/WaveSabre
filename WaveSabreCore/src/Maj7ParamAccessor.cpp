
#include <WaveSabreCore/Maj7ParamAccessor.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		float ParamAccessor::GetRawVal__(int offset) const {
			return mParamCache[mBaseParamID + offset];
		}

		float ParamAccessor::GetN11Value__(int offset, float mod) const {
			float ret = math::clampN11((GetRawVal__(offset) * 2 - 1) + mod);
			return ret;
		}

		float ParamAccessor::Get01Value__(int offset, float mod) const {
			return math::clamp01(GetRawVal__(offset) + mod);
		}

		float ParamAccessor::GetBoolValue__(int offset) const {
			return (GetRawVal__(offset) > 0.5f);
		}
		int ParamAccessor::GetIntValue__(int offset, const IntParamConfig& cfg) const {
			int c = cfg.GetDiscreteValueCount();// maxValIncl - minValIncl + 1;
			int r = (int)(GetRawVal__(offset) * c);
			r = math::ClampI(r, 0, c - 1);
			return r + cfg.mMinValInclusive;
		}
		void ParamAccessor::SetRawVal__(int offset, float v) const {
			mParamCache[mBaseParamID + offset] = v;
		}
		void ParamAccessor::SetIntValue__(int offset, const IntParamConfig& cfg, int val) {
			int c = cfg.GetDiscreteValueCount();
			real_t p = real_t(val);// +0.5f; // so it lands in the middle of a bucket.
			p += 0.5f - cfg.mMinValInclusive;
			p /= c;
			SetRawVal__(offset, p);
		}
		void ParamAccessor::SetN11Value__(int offset, float v) const {
			SetRawVal__(offset, (v + 1) * 0.5f);
		}

		float ParamAccessor::GetEnvTimeMilliseconds__(int offset, float mod) const
		{
			float param = this->GetRawVal__(offset) + mod; // apply current modulation value.
			param = math::clamp01(param);
			param -= 0.5f;       // -.5 to .5
			param *= gEnvTimeRangeLog2; // -5 to +5 (2^-5 = .0312; 2^5 = 32), with 375ms center val means [12ms, 12sec]
			float fact = math::pow2_N16_16(param);
			param = gEnvTimeCenterValue * fact;
			param -= gEnvTimeMinRawVal; // pow(2,x) doesn't ever reach 0 value. subtracting the min allows 0 to exist.
			return math::clamp(param, gEnvTimeMinRealVal, gEnvTimeMaxRealVal);
		}

		float ParamAccessor::ApplyCurveToValue__(int offset, float x, float modVal) const {
			float k = GetN11Value__(offset, 0) + modVal; // TODO: shouldn't we stick the mod val in there?
			if (k < 0.0001 && k > -0.0001) return x; // speed optimization; most curves being processed are flat so skip the lookup entirely.
			return math::modCurve_xN11_kN11(x, k);
		}



		float ParamAccessor::GetScaledRealValue__(int offset, float minIncl, float maxIncl, float mod) const
		{
			float range = maxIncl - minIncl;
			float r = Get01Value__(offset, mod) * range;
			//	real_t r = mParamValue * range;
			//	r = math::clamp(r, 0, range);
			//	return r + mMinValueInclusive;
			return r + minIncl;
		}
		void ParamAccessor::SetRangedValue__(int offset, float minIncl, float maxIncl, float value)
		{
			float range = maxIncl - minIncl;
			this->SetRawVal__(offset, (value - minIncl) / range);
			//	mParamValue = (val - mMinValueInclusive) / GetRange();
		}


		/*static*/ float ParamAccessor::ParamToLinearVolume__(float maxLinear, float x)
		{
			if (x <= 0)
				return 0;
			return x * x * maxLinear;
		}
		/*static*/ float ParamAccessor::LinearToParamVolume__(float maxLinear, float x)
		{
			return math::sqrt(x / maxLinear);
		}
		/*static*/ float ParamAccessor::ParamToDecibelsVolume__(float maxLinear, float x)
		{
			return math::LinearToDecibels(ParamToLinearVolume__(maxLinear, x));
		}
		/*static*/ float ParamAccessor::DecibelsToParamVolume__(float maxLinear, float db)
		{
			return LinearToParamVolume__(maxLinear, math::DecibelsToLinear(db));
		}

		float ParamAccessor::GetLinearVolume__(int offset, const VolumeParamConfig& cfg, float mod) const
		{
			return ParamToLinearVolume__(cfg.mMaxValLinear, Get01Value__(offset, mod));
		}
		float ParamAccessor::GetDecibels__(int offset, const VolumeParamConfig& cfg, float mod) const
		{
			return ParamToDecibelsVolume__(cfg.mMaxValLinear, Get01Value__(offset, mod));
		}
		void ParamAccessor::SetLinearVolume__(int offset, const VolumeParamConfig& cfg, float f)
		{
			SetRawVal__(offset, LinearToParamVolume__(cfg.mMaxValLinear, f));
		}
		void ParamAccessor::SetDecibels__(int offset, const VolumeParamConfig& cfg, float db)
		{
			SetRawVal__(offset, DecibelsToParamVolume__(cfg.mMaxValLinear, db));
		}

		bool ParamAccessor::IsSilentVolume__(int offset, const VolumeParamConfig& cfg) const
		{
			return math::IsSilentGain(GetLinearVolume__(offset, cfg, 0));
		}


		//explicit FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/);
		//// noteHz is the playing note, to support key-tracking.
		//float GetFrequency(float noteHz, float paramModulation) const;
		//void SetFrequencyAssumingNoKeytracking(float hz);
		//// param modulation is normal krate param mod
		//// noteModulation includes osc.mPitchFine + osc.mPitchSemis + detune;
		//float GetMidiNote(float playingMidiNote, float paramModulation) const;

		float ParamAccessor::GetFrequency__(int freqOffset, int ktOffset, const FreqParamConfig& cfg, float noteHz, float mod) const
		{
			float param = Get01Value__(freqOffset, mod);// mValue.Get01Value() + paramModulation; // apply current modulation value.
			// at 0.5, we use 1khz.
			// for each 0.1 param value, it's +/- one octave

			//float centerFreq = 1000; // the cutoff frequency at 0.5 param value.

			// with no KT,
			// so if param is 0.8, we want to multiply by 8 (2^3)
			// if param is 0.3, multiply by 1/4 (2^(1/4))

			// with full KT,
			// at 0.3, we use playFrequency.
			// for each 0.1 param value, it's +/- one octave.
			// to copy massive, 1:1 is at paramvalue 0.3. 0.5 is 2 octaves above playing freq.
			float ktFreq = noteHz * 4;
			float ktParamVal = (ktOffset < 0) ? 0 : Get01Value__(ktOffset, 0);
			float centerFreq = math::lerp(cfg.mCenterFrequency, ktFreq, ktParamVal);

			param -= 0.5f;  // signed distance from 0.5 -.2 (0.3 = -.2, 0.8 = .3)   [-.5,+.5]
			param *= cfg.mScale;// 10.0f; // (.3 = -2, .8 = 3) [-15,+15]
			//float fact = math::pow(2, param);
			float fact = math::pow2_N16_16(param);
			return math::clamp(centerFreq * fact, 0.0f, 22050.0f);
		}

		// param modulation is normal krate param mod
		// noteModulation includes osc.mPitchFine + osc.mPitchSemis + detune;
		float ParamAccessor::GetMidiNote__(int freqOffset, int ktOffset, const FreqParamConfig& cfg, float playingMidiNote, float mod) const
		{
			float ktNote = playingMidiNote + 24; // center represents playing note + 2 octaves.

			float centerNote = math::lerp(cfg.mCenterMidiNote, ktNote, Get01Value__(ktOffset, 0));

			float param = Get01Value__(freqOffset, mod);

			param = (param - 0.5f) * cfg.mScale;// 10; // rescale from 0-1 to -5 to +5 (octaves)
			float paramSemis =
				centerNote + param * 12; // each 1 param = 1 octave. because we're in semis land, it's just a mul.
			return paramSemis;
		}
		void ParamAccessor::SetFrequencyAssumingNoKeytracking__(int freqOffset, const FreqParamConfig& cfg, float hz)
		{
			// 2 ^ param
			float  p = math::log2(hz);
			p /= cfg.mScale;
			p += 0.5f;
			//this->mValue.SetParamValue(math::clamp01(p));
			this->SetRawVal__(freqOffset, p);
		}

	} // namespace M7


} // namespace WaveSabreCore



