#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		// https://www.desmos.com/calculator/ipgewtox2e
		// so far all time params have similar range which mimics NI Massive. but when we need other time ranges, it's time to 
		// rethink, and be more like freq param.
		struct PowCurvedParamCfg {

			// from very small but not zero (e.g. 0.0001), as high as you want. use desmos link to visualize. higher values (10+?) make a more linear range.
			// k = 10 pretty much matches the NI Massive time param curve.
			const float mK;
			const float mMinMSPlusK;
			const float mBase;

			// the formula to map min/max/k to value over 0-1 param range,
			// min += k;
			// max += k;
			// R = max/min;
			// min * (R ^ x) - k
			constexpr PowCurvedParamCfg(float minMS, float maxMS, float k) : //
				mK(k),
				mMinMSPlusK(minMS + k), //
				mBase((maxMS + k) / mMinMSPlusK)
			{
			}

			float Param01ToValue(float p01) const {
				p01 = math::clamp01(p01);
				// min * (R ^ x) - k
				return mMinMSPlusK * M7::math::pow(mBase, p01) - mK;
			}

			float ValueToParam01(float ms) const {
				float t = ms + mK;
				t /= mMinMSPlusK;
				float n = M7::math::log10(t);
				n /= M7::math::log10(mBase);
				return math::clamp01(n);
			}
		};

		static constexpr PowCurvedParamCfg gEnvTimeCfg{ 0.0f, 12000.0f, 12.0f }; // value of K can be found by looking in param explorer in a vst.

		// something like compressor ratio requires a very steep curve, and using 1/x will fit that. it's based off the principle curve 1/(1-x).
		struct DivCurvedParamCfg {
			// k should be > 1, but makes little difference over about 2. small k means a steep curve.
			const float mK;
			const float mMin;
			const float mMax;

			const float mKMinus1;
			const float mKTimesKMinus1;

			// to calc 01 param to 01 curved:
			// ((k * (k - 1)) / (k - x)) - (k - 1)
			// OR,
			// km1 = k - 1;
			// kmx = k - x;
			// k * km1 / kmx - km1
			constexpr DivCurvedParamCfg(float minVal, float maxVal, float k) : //
				mK(k),
				mMin(minVal), //
				mMax(maxVal),//
				mKMinus1(k - 1),
				mKTimesKMinus1(k * (k - 1))
			{
			}

			float Param01ToValue(float p01) const {
				p01 = math::clamp01(p01);
				float t = mKTimesKMinus1 / (mK - p01) - mKMinus1;
				return M7::math::lerp(mMin, mMax, t);
			}

			float ValueToParam01(float v) const {
				float t = M7::math::lerp_rev(mMin, mMax, v);
				// solving the above for x leads to:
				// k*t/(k + t - 1)
				return mK * t / (mK + t - 1);
			}
		};

		static constexpr M7::DivCurvedParamCfg gBiquadFilterQCfg{ 0.2f, 12.0f, 1.1f };

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

			float Get01Value__(int offset) const;
			template<typename T>
			float Get01Value(T offset) const {
				static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "");
				return Get01Value__((int)offset);
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

			//static constexpr real_t gEnvTimeCenterValue = 375; // the MS at 0.5 param value.
			//static constexpr int gEnvTimeRangeLog2 = 10;      // must be even for below calculations to work
			//static constexpr real_t gEnvTimeMinRawVal = 12; // slightly above the correct value (~1ms), in order to guarantee envelope times can reach true 0.
			////(1.0f / (1 << (gRangeLog2 / 2))) *
			////gCenterValue; // minimum value as described by log calc (before subtracting min).
			//static constexpr real_t gEnvTimeMaxRawVal = (1 << (gEnvTimeRangeLog2 / 2)) * gEnvTimeCenterValue;
			//static constexpr real_t gEnvTimeMinRealVal = 0;
			//static constexpr real_t gEnvTimeMaxRealVal = gEnvTimeMaxRawVal - gEnvTimeMinRawVal;

			//float GetEnvTimeMilliseconds__(int offset, float mod) const;
			//template<typename Toffset>
			//float GetEnvTimeMilliseconds(Toffset offset, float mod) const
			//{
			//	static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
			//	return GetEnvTimeMilliseconds__((int)offset, mod);
			//}

			float GetPowCurvedValue__(int offset, const PowCurvedParamCfg& cfg, float mod) const;
			template<typename Toffset>
			float GetPowCurvedValue(Toffset offset, const PowCurvedParamCfg& cfg, float mod) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetPowCurvedValue__((int)offset, cfg, mod);
			}

			void SetPowCurvedValue__(int offset, const PowCurvedParamCfg& cfg, float ms);

			template<typename Toffset>
			void SetPowCurvedValue(Toffset offset, const PowCurvedParamCfg& cfg, float ms)
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				SetPowCurvedValue__((int)offset, cfg, ms);
			}

			float GetDivCurvedValue__(int offset, const DivCurvedParamCfg& cfg, float mod) const;
			template<typename Toffset>
			float GetDivCurvedValue(Toffset offset, const DivCurvedParamCfg& cfg, float mod = 0) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetDivCurvedValue__((int)offset, cfg, mod);
			}

			void SetDivCurvedValue__(int offset, const DivCurvedParamCfg& cfg, float v);

			template<typename Toffset>
			void SetDivCurvedValue(Toffset offset, const DivCurvedParamCfg& cfg, float v)
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				SetDivCurvedValue__((int)offset, cfg, v);
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
			float GetScaledRealValue(Toffset offset, float minIncl, float maxIncl, float mod = 0) const {
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
			float GetFrequency__(int freqOffset, const FreqParamConfig& cfg) const;
			template<typename Toffset>
			float GetFrequency(Toffset freqOffset, const FreqParamConfig& cfg) const
			{
				static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
				return GetFrequency__((int)freqOffset, cfg);
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


			//float GetWSQValue__(int offset) const;
			//template<typename Toffset>
			//float GetWSQValue(Toffset offset) const
			//{
			//	static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
			//	return GetWSQValue__((int)offset);
			//}

			//void SetWSQValue__(int offset, float q);
			//template<typename Toffset>
			//void SetWSQValue(Toffset offset, float q) const
			//{
			//	static_assert(std::is_integral_v<Toffset> || std::is_enum_v<Toffset>, "");
			//	SetWSQValue__((int)offset, q);
			//}

		}; // paramaccessor

		// convenience for VST only, a small param cache and param accessor sorta combined.
		// TODO...
		// the idea is to avoid having this very verbose:
		// float backing = paramValue;
		// ParamAccessor pa {&backing, 0};
		// int x = pa.GetIntVal(0, intcfg);
		//
		// into something more like:
		// QuickParam qp{paramValue};
		// int x = qp.GetIntVal(intcfg);
		struct QuickParam
		{
		private:
			bool mHasValue = false;
			float mBacking;
			ParamAccessor mAccessor{ &mBacking, 0 };

			bool mHasIntCfg = false;
			IntParamConfig mIntCfg{ 0, 1 };

			bool mHasFreqCfg = false;
			FreqParamConfig mFreqCfg{ gFilterFreqConfig };

			bool mHasPowCurvedCfg = false;
			PowCurvedParamCfg mPowCurvedCfg{ 0.0f, 500.0f, 9 };


			bool mHasDivCurvedCfg = false;
			DivCurvedParamCfg mDivCurvedCfg{ 1, 50, 1.05f };

			bool mHasVolumeCfg = false;
			VolumeParamConfig mVolumeCfg{ 1.9952623149688795f, 6.0f };
		public:

			explicit QuickParam(float val01) : mBacking(val01), mHasValue(true) {
			}
			QuickParam() {
			}

			QuickParam(float val01, const IntParamConfig& cfg) : mHasValue(true), mBacking(val01), mHasIntCfg(true), mIntCfg(cfg) {
			}
			explicit QuickParam(const IntParamConfig& cfg) : mHasIntCfg(true), mIntCfg(cfg) {
			}

			QuickParam(float val01, const FreqParamConfig& cfg) : mHasValue(true), mBacking(val01), mHasFreqCfg(true), mFreqCfg(cfg) {
			}
			explicit QuickParam(const FreqParamConfig& cfg) : mHasFreqCfg(true), mFreqCfg(cfg) {
			}

			QuickParam(float val01, const PowCurvedParamCfg& cfg) : mHasValue(true), mBacking(val01), mHasPowCurvedCfg(true), mPowCurvedCfg(cfg) {
			}
			explicit QuickParam(const PowCurvedParamCfg& cfg) : mHasPowCurvedCfg(true), mPowCurvedCfg(cfg) {
			}

			QuickParam(float val01, const DivCurvedParamCfg& cfg) : mHasValue(true), mBacking(val01), mHasDivCurvedCfg(true), mDivCurvedCfg(cfg) {
			}
			explicit QuickParam(const DivCurvedParamCfg& cfg) : mHasDivCurvedCfg(true), mDivCurvedCfg(cfg) {
			}

			QuickParam(float val01, const VolumeParamConfig& cfg) : mHasValue(true), mBacking(val01), mHasVolumeCfg(true), mVolumeCfg(cfg) {
			}
			explicit QuickParam(const VolumeParamConfig& cfg) : mHasVolumeCfg(true), mVolumeCfg(cfg) {
			}

			// RAW -------------------------------------------------------------------------------
			void SetRawValue(float val01) {
				mHasValue = true;
				mBacking = val01;
			}
			float GetRawValue() const {
				CCASSERT(!!mHasValue);
				return mBacking;
			}

			// INT -------------------------------------------------------------------------------
			int GetIntValue() const {
				CCASSERT(!!mHasValue);
				CCASSERT(!!mHasIntCfg);
				return mAccessor.GetIntValue(0, mIntCfg);
			}
			int GetIntValue(const IntParamConfig& cfg) const {
				CCASSERT(!!mHasValue);
				return mAccessor.GetIntValue(0, cfg);
			}
			float SetIntValue(int n) {
				CCASSERT(!!mHasIntCfg);
				mHasValue = true;
				mAccessor.SetIntValue(0, mIntCfg, n);
				return mBacking;
			}
			float SetIntValue(const IntParamConfig& cfg, int n) {
				mHasValue = true;
				mAccessor.SetIntValue(0, cfg, n);
				return mBacking;
			}

			// ENUM -------------------------------------------------------------------------------
			template<typename Tenum>
			Tenum GetEnumValue() const {
				CCASSERT(!!mHasValue);
				return mAccessor.GetEnumValue<Tenum>(0);
			}
			template<typename Tenum>
			float SetEnumValue(Tenum val) {
				mAccessor.SetEnumValue(0, val);
				mHasValue = true;
				return mBacking;
			}

			// BOOL -------------------------------------------------------------------------------
			bool GetBoolValue() const {
				CCASSERT(!!mHasValue);
				return mAccessor.GetBoolValue(0);
			}

			// N11 (-1 to 1 range) -------------------------------------------------------------------------------
			float GetN11Value() const {
				CCASSERT(!!mHasValue);
				return mAccessor.GetN11Value(0, 0);
			}
			float SetN11Value(float n11) {
				mHasValue = true;
				mAccessor.SetN11Value(0, n11);
				return mBacking;
			}

			// FREQUENCY -------------------------------------------------------------------------------
			float SetFrequencyAssumingNoKeytracking(float hz) {
				CCASSERT(!!mHasFreqCfg);
				mHasValue = true;
				mAccessor.SetFrequencyAssumingNoKeytracking(0, mFreqCfg, hz);
				return mBacking;
			}
			float SetFrequencyAssumingNoKeytracking(const FreqParamConfig& cfg, float hz) {
				mHasValue = true;
				mAccessor.SetFrequencyAssumingNoKeytracking(0, cfg, hz);
				return mBacking;
			}

			float GetFrequency() const {
				CCASSERT(!!mHasValue);
				CCASSERT(!!mHasFreqCfg);
				return mAccessor.GetFrequency(0, mFreqCfg);
			}


			// SCALED (arbitrary float range) -------------------------------------------------------------------------------
			float GetScaledValue(float vmin, float vmax) const {
				CCASSERT(!!mHasValue);
				return mAccessor.GetScaledRealValue(0, vmin, vmax);
			}
			float SetScaledValue(float vmin, float vmax, float val) {
				mHasValue = true;
				mAccessor.SetRangedValue(0, vmin, vmax, val);
				return mBacking;
			}

			// POW-CURVED -------------------------------------------------------------------------------
			float GetPowCurvedValue() const {
				CCASSERT(mHasValue);
				CCASSERT(mHasPowCurvedCfg);
				return mAccessor.GetPowCurvedValue(0, mPowCurvedCfg, 0);
			}
			float SetPowCurvedValue(float val) {
				CCASSERT(mHasPowCurvedCfg);
				mHasValue = true;
				mAccessor.SetPowCurvedValue(0, mPowCurvedCfg, val);
				return mBacking;
			}
			float GetPowCurvedValue(const PowCurvedParamCfg& cfg) const {
				CCASSERT(mHasValue);
				return mAccessor.GetPowCurvedValue(0, cfg, 0);
			}
			float SetPowCurvedValue(const PowCurvedParamCfg& cfg, float val) {
				mHasValue = true;
				mAccessor.SetPowCurvedValue(0, cfg, val);
				return mBacking;
			}

			// DIV-CURVED -------------------------------------------------------------------------------
			float GetDivCurvedValue() const {
				CCASSERT(mHasValue);
				CCASSERT(mHasDivCurvedCfg);
				return mAccessor.GetDivCurvedValue(0, mDivCurvedCfg, 0);
			}
			float SetDivCurvedValue(float val) {
				CCASSERT(mHasDivCurvedCfg);
				mHasValue = true;
				mAccessor.SetDivCurvedValue(0, mDivCurvedCfg, val);
				return mBacking;
			}
			float GetDivCurvedValue(const DivCurvedParamCfg& cfg) const {
				CCASSERT(mHasValue);
				return mAccessor.GetDivCurvedValue(0, cfg, 0);
			}
			float SetDivCurvedValue(const DivCurvedParamCfg& cfg, float val) {
				mHasValue = true;
				mAccessor.SetDivCurvedValue(0, cfg, val);
				return mBacking;
			}


			// VOLUME (-inf to N) -------------------------------------------------------------------------------
			float GetVolumeLin() const {
				CCASSERT(mHasValue);
				CCASSERT(mHasVolumeCfg);
				return mAccessor.GetLinearVolume(0, mVolumeCfg);
			}
			float SetVolumeLin(float val) {
				CCASSERT(mHasVolumeCfg);
				mHasValue = true;
				mAccessor.SetLinearVolume(0, mVolumeCfg, val);
				return mBacking;
			}
			float GetVolumeLin(const VolumeParamConfig& cfg) const {
				CCASSERT(mHasValue);
				return mAccessor.GetLinearVolume(0, cfg);
			}
			float SetVolumeLin(const VolumeParamConfig& cfg, float val) {
				mHasValue = true;
				mAccessor.SetLinearVolume(0, cfg, val);
				return mBacking;
			}

			float GetVolumeDB() const {
				return math::LinearToDecibels(GetVolumeLin());
			}
			float SetVolumeDB(float val) {
				val = math::DecibelsToLinear(val);
				return SetVolumeLin(val);
			}
			float GetVolumeDB(const VolumeParamConfig& cfg) const {
				return math::LinearToDecibels(GetVolumeLin(cfg));
			}
			float SetVolumeDB(const VolumeParamConfig& cfg, float val) {
				val = math::DecibelsToLinear(val);
				return SetVolumeLin(cfg, val);
			}

		};


	} // namespace M7


} // namespace WaveSabreCore



