#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ParamAccessor.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		struct ParamAccessor
		{
			static constexpr size_t MaxEnumItems = 2023;

			float* const mParamCache;
			const int mBaseParamID;

			explicit ParamAccessor(const ParamAccessor& rhs) :
				mParamCache(rhs.mParamCache),
				mBaseParamID(rhs.mBaseParamID)
			{
			}

			template<typename T>
			explicit ParamAccessor(const ParamAccessor& rhs, T baseParamID) :
				mParamCache(rhs.mParamCache),
				mBaseParamID((int)baseParamID)
			{
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
			}

			template<typename T>
			ParamAccessor(float* const paramCache, T baseParamID) :
				mParamCache(paramCache),
				mBaseParamID((int)baseParamID)
			{
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
			}

			float* GetOffsetParamCache() { return mParamCache + mBaseParamID; }

			template<typename Toffset>
			int GetParamIndex(Toffset offset) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return mBaseParamID + (int)offset;
			}

			float GetRawVal__(int offset) const;
			template<typename T>
			float GetRawVal(T offset) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				return GetRawVal__((int)offset);
			}

			float GetN11Value__(int offset, float mod) const;
			template<typename T>
			float GetN11Value(T offset, float mod) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				return GetN11Value__((int)offset, mod);
			}

			float Get01Value__(int offset, float mod) const;
			template<typename T>
			float Get01Value(T offset, float mod) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				return Get01Value__((int)offset, mod);
			}

			bool GetBoolValue__(int offset) const;
			template<typename T>
			bool GetBoolValue(T offset) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				return GetBoolValue__((int)offset);
			}

			int GetIntValue__(int offset, const IntParamConfig& cfg) const;
			template<typename T>
			int GetIntValue(T offset, const IntParamConfig& cfg) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				return GetIntValue__((int)offset, cfg);
			}

			static constexpr IntParamConfig gEnumIntConfig{ 0, MaxEnumItems };

			template<typename Tenum, typename Toffset>
			Tenum GetEnumValue(Toffset offset) const {
				static_assert(std::is_enum_v<Tenum>, "");
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return (Tenum)GetIntValue__((int)offset, gEnumIntConfig);
			}

			static constexpr real_t gEnvTimeCenterValue = 375; // the MS at 0.5 param value.
			static constexpr int gEnvTimeRangeLog2 = 10;      // must be even for below calculations to work
			static constexpr real_t gEnvTimeMinRawVal = 12; // slightly above the correct value (~1ms), in order to guarantee envelope times can reach true 0.
			//(1.0f / (1 << (gRangeLog2 / 2))) *
			//gCenterValue; // minimum value as described by log calc (before subtracting min).
			static constexpr real_t gEnvTimeMaxRawVal = (1 << (gEnvTimeRangeLog2 / 2)) * gEnvTimeCenterValue;
			static constexpr real_t gEnvTimeMinRealVal = 0;
			static constexpr real_t gEnvTimeMaxRealVal = gEnvTimeMaxRawVal - gEnvTimeMinRawVal;

			float GetEnvTimeMilliseconds__(int offset, float mod) const;
			template<typename Toffset>
			float GetEnvTimeMilliseconds(Toffset offset, float mod) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetEnvTimeMilliseconds__((int)offset, mod);
			}

			float ParamAccessor::ApplyCurveToValue__(int offset, float x, float modVal) const;
			template<typename Toffset>
			real_t ApplyCurveToValue(Toffset offset, real_t x, real_t mod = 0.0f) const {
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return ApplyCurveToValue__((int)offset, x, mod);
			}

			void SetRawVal__(int offset, float v) const;
			template<typename T>
			void SetRawVal(T offset, float v) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				SetRawVal__((int)offset, v);
			}

			// this is mostly a symbolic function; we don't clamp when setting values so this just goes to RawVal directly.
			template<typename T>
			void Set01Val(T offset, float v) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				SetRawVal__((int)offset, v);
			}


			template<typename T>
			void SetBoolValue(T offset, bool v)
			{
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				SetRawVal__((int)offset, v ? 1.0f : 0.0f);
			}

			void SetIntValue__(int offset, const IntParamConfig& cfg, int val);
			template<typename T>
			void SetIntValue(T offset, const IntParamConfig& cfg, int val) {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				SetIntValue__((int)offset, cfg, val);
			}

			template<typename Tenum, typename Toffset>
			void SetEnumValue(Toffset offset, Tenum val)
			{
				static_assert(std::is_enum_v<Tenum>, "");
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				SetIntValue__((int)offset, gEnumIntConfig, (int)val);
			}

			// allows setting an enum value, as an integer, without having to cast.
			template<typename T>
			void SetEnumIntValue(T offset, int val) {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				SetIntValue__((int)offset, gEnumIntConfig, val);
			}

			void SetN11Value__(int offset, float v) const;
			template<typename T>
			void SetN11Value(T offset, float v) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				SetN11Value__((int)offset, v);
			}

			float GetScaledRealValue__(int offset, float minIncl, float maxIncl, float mod) const;
			template<typename Toffset>
			float GetScaledRealValue(Toffset offset, float minIncl, float maxIncl, float mod) const {
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetScaledRealValue__((int)offset, minIncl, maxIncl, mod);
			}

			void SetRangedValue__(int offset, float minIncl, float maxIncl, float value);
			template<typename Toffset>
			void SetRangedValue(Toffset offset, float minIncl, float maxIncl, float value) {
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				SetRangedValue__((int)offset, minIncl, maxIncl, value);
			}

			// a volume param which goes from silent to a maxdb value
			static float ParamToLinearVolume__(float maxLinear, float x);
			static float LinearToParamVolume__(float maxLinear, float x);
			static float ParamToDecibelsVolume__(float maxLinear, float x);
			static float DecibelsToParamVolume__(float maxLinear, float db);

			float GetLinearVolume__(int offset, const VolumeParamConfig& cfg, float mod) const;
			float GetDecibels__(int offset, const VolumeParamConfig& cfg, float mod) const;
			void SetLinearVolume__(int offset, const VolumeParamConfig& cfg, float f);
			void SetDecibels__(int offset, const VolumeParamConfig& cfg, float db);

			template<typename Toffset>
			float GetLinearVolume(Toffset offset, const VolumeParamConfig& cfg, float mod = 0.0f) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetLinearVolume__((int)offset, cfg, mod);
			}
			template<typename Toffset>
			float GetDecibels(Toffset offset, const VolumeParamConfig& cfg, float mod = 0.0f) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetDecibels__((int)offset, cfg, mod);
			}
			template<typename Toffset>
			void SetLinearVolume(Toffset offset, const VolumeParamConfig& cfg, float f)
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				SetLinearVolume__((int)offset, cfg, f);
			}
			template<typename Toffset>
			void SetDecibels(Toffset offset, const VolumeParamConfig& cfg, float db)
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				SetDecibels__((int)offset, cfg, db);
			}
			bool IsSilentVolume__(int offset, const VolumeParamConfig& cfg) const;
			template<typename Toffset>
			bool  IsSilentVolume(Toffset offset, const VolumeParamConfig& cfg) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return IsSilentVolume__((int)offset, cfg);
			}


			//explicit FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/);
			// noteHz is the playing note, to support key-tracking.
			float GetFrequency__(int freqOffset, int ktOffset, const FreqParamConfig& cfg, float noteHz, float mod) const;
			template<typename Toffset, typename TktOffset>
			float GetFrequency(Toffset freqOffset, TktOffset ktOffset /* -1 means no kt */, const FreqParamConfig& cfg, float noteHz, float mod) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				static_assert(std::is_integral_v<TktOffset> || std::is_enum_v<TktOffset>, "");
				return GetFrequency__((int)freqOffset, (int)ktOffset, cfg, noteHz, mod);
			}

			// param modulation is normal krate param mod
			// noteModulation includes osc.mPitchFine + osc.mPitchSemis + detune;
			float GetMidiNote__(int freqOffset, int ktOffset, const FreqParamConfig& cfg, float playingMidiNote, float mod) const;
			template<typename Toffset>
			float GetMidiNote(Toffset freqOffset, Toffset ktOffset, const FreqParamConfig& cfg, float playingMidiNote, float mod) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetMidiNote__((int)freqOffset, (int)ktOffset, cfg, noteHz, mod);
			}

			void SetFrequencyAssumingNoKeytracking__(int freqOffset, const FreqParamConfig& cfg, float hz);

			template<typename Toffset>
			void SetFrequencyAssumingNoKeytracking(Toffset freqOffset, const FreqParamConfig& cfg, float hz)
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				SetFrequencyAssumingNoKeytracking__((int)freqOffset, cfg, hz);
			}


			float GetWSQValue__(int offset) const;
			template<typename Toffset>
			float GetWSQValue(Toffset offset) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetWSQValue__((int)offset);
			}

			void SetWSQValue__(int offset, float q);
			template<typename Toffset>
			void SetWSQValue(Toffset offset, float q) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				SetWSQValue__((int)offset, q);
			}

		};

	} // namespace M7


} // namespace WaveSabreCore



