
#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		void Init() {
			if (math::gCrtFns) return;
			math::gCrtFns = new math::CrtFns();
			math::gLuts = new math::LUTs();
		}

		static uint16_t gAudioRecalcSampleMaskValues[] = {
			127, // Potato,
			63, // Carrot,
			31, // Cauliflower,
			7, // Celery,
			1, // Artichoke,
		};

		static uint16_t gModulationRecalcSampleMaskValues[] = {
			127, // Potato,
			63, // Carrot,
			31, // Cauliflower,
			15, // Celery,
			7, // Artichoke,
		};

		uint16_t gAudioOscRecalcSampleMask = gAudioRecalcSampleMaskValues[(size_t)QualitySetting::Celery];
		uint16_t gModulationRecalcSampleMask = gModulationRecalcSampleMaskValues[(size_t)QualitySetting::Celery];
		QualitySetting gQualitySetting = QualitySetting::Celery;

		uint16_t GetAudioOscillatorRecalcSampleMask()
		{
			return gAudioOscRecalcSampleMask;
		}
		uint16_t GetModulationRecalcSampleMask()
		{
			return gModulationRecalcSampleMask;
		}

		void SetQualitySetting(QualitySetting n)
		{
			gAudioOscRecalcSampleMask = gAudioRecalcSampleMaskValues[(size_t)n];
			gModulationRecalcSampleMask = gModulationRecalcSampleMaskValues[(size_t)n];
			gQualitySetting = n;
		}
		QualitySetting GetQualitySetting()
		{
			return gQualitySetting;
		}


		namespace math
		{
			CrtFns* gCrtFns = nullptr;
			LUTs* gLuts = nullptr;

			LUTs::LUTs() :
				gSinLUT{ 768, [](float x) {
				//static CrtMathFn gfn{ "sin" };
				//return (float)gfn.invoke((double)x * 2 * M_PI);
				return (float)math::CrtSin((double)x * 2 * M_PI);

				//return (float)::sin((double)x * 2 * M_PI);
			} },
			gCosLUT{ 768,  [](float x)
				{
					//return (float)::cos((double)x * 2 * M_PI);
					return (float)math::CrtCos((double)x * 2 * M_PI);

			} },
			gTanhLUT{ 768 },
			gSqrt01LUT{ 768, [](float x) {
				//return ::sqrtf(x);
				return (float)math::CrtPow((double)x, 0.5);
			} },

			gCurveLUT{ 768 },
			gPow2_N16_16_LUT{ 768 }

			{}

			bool FloatEquals(real_t f1, real_t f2, real_t eps)
			{
				return math::abs(f1 - f2) < eps;
			}

			bool FloatLessThanOrEquals(real_t lhs, real_t rhs, real_t eps)
			{
				return lhs <= (rhs + eps);
			}

			float lerp(float a, float b, float t)
			{
				return a * (1.0f - t) + b * t;
			}

			real_t CalculateInc01PerSampleForMS(real_t ms)
			{
				return clamp01(1000.0f / (math::max(0.01f, ms) * (real_t)Helpers::CurrentSampleRate));
			}

			bool DoesEncounter(double t1, double t2, float x) {
				if (t1 < t2) {
					return (t1 < x&& x <= t2);
				}
				return (x <= t2 || x > t1);
			}



			LUT01::LUT01(size_t nSamples, float (*fn)(float)) :
				mNSamples(nSamples),
				mpTable(new float[nSamples])
			{
				// populate table.
				for (size_t i = 0; i < nSamples; ++i) {
					float x = float(i) / (nSamples - 1);
					mpTable[i] = fn(x);
				}
			}

#pragma message("Leaking memory to save bits.")
			//LUT01::~LUT01() {
			//    delete[] mpTable;
			//}

			float LUT01::Invoke(float x) const {
				if (x <= 0) return mpTable[0];
				if (x >= 1) return mpTable[mNSamples - 1];
				float index = x * (mNSamples - 1);
				int lower_index = static_cast<int>(index);
				int upper_index = lower_index + 1;
				float t = index - lower_index;
				return (1 - t) * mpTable[lower_index] + t * mpTable[upper_index];
			}

			SinCosLUT::SinCosLUT(size_t nSamples, float (*fn)(float))
				: LUT01(nSamples, fn)
			{}

			float SinCosLUT::Invoke(float x) const {
				static constexpr float gPeriodCorrection = 1 / gPITimes2;
				float scaledX = x * gPeriodCorrection;
				return LUT01::Invoke(fract(scaledX));
			}


			Pow2_N16_16_LUT::Pow2_N16_16_LUT(size_t nSamples)
				: LUT01(nSamples, [](float n) {
				return (float)math::CrtPow(2.0, ((double)n - .5) * 32); // return (float)::sin((double)x * 2 * M_PI); }};
					})
			{}

			float Pow2_N16_16_LUT::Invoke(float x) const { // passed in value is -16,16
				static constexpr float gScaleCorrection = 1.0f / 32;
				x *= gScaleCorrection;
				x += 0.5f;
				return LUT01::Invoke(x);
			}




			// tanh approaches -1 before -PI, and +1 after +PI. so squish the range and do a 0,1 LUT mapping from -PI,PI
			TanHLUT::TanHLUT(size_t nSamples)
				: LUT01(nSamples, [](float x) { return (float)::tanh(((double)x - .5) * M_PI * 2); })
			{}

			float TanHLUT::Invoke(float x) const {
				static constexpr float gOneOver2pi = 1.0f / gPITimes2;
				x *= gOneOver2pi;
				x += 0.5f;
				return LUT01::Invoke(x);
			}

			LUT2D::LUT2D(size_t nSamplesX, size_t nSamplesY, float (*fn)(float, float))
				: mNSamplesX(nSamplesX)
				, mNSamplesY(nSamplesY)
				, mpTable(new float[nSamplesX * nSamplesY])
			{
				// Populate table.
				for (size_t y = 0; y < nSamplesY; ++y) {
					for (size_t x = 0; x < nSamplesX; ++x) {
						float u = float(x) / (nSamplesX - 1);
						float v = float(y) / (nSamplesY - 1);
						mpTable[y * nSamplesX + x] = fn(u, v);
					}
				}
			}

#pragma message("Leaking memory to save bits.")
			//LUT2D::~LUT2D() {
			//    delete[] mpTable;
			//}

			float LUT2D::Invoke(float x, float y) const {
				if (x <= 0) x = 0;
				if (x >= 1) x = 0.9999f; // this ensures it will not OOB
				if (y <= 0) y = 0;
				if (y >= 1) y = 0.9999f;

				float indexX = x * (mNSamplesX - 1);
				float indexY = y * (mNSamplesY - 1);

				int lowerIndexX = static_cast<int>(indexX);
				int lowerIndexY = static_cast<int>(indexY);
				int upperIndexX = lowerIndexX + 1;
				int upperIndexY = lowerIndexY + 1;

				float tx = indexX - lowerIndexX;
				float ty = indexY - lowerIndexY;

				float f00 = mpTable[lowerIndexY * mNSamplesX + lowerIndexX];
				float f01 = mpTable[upperIndexY * mNSamplesX + lowerIndexX];
				float f10 = mpTable[lowerIndexY * mNSamplesX + upperIndexX];
				float f11 = mpTable[upperIndexY * mNSamplesX + upperIndexX];

				float f0 = (1 - tx) * f00 + tx * f10;
				float f1 = (1 - tx) * f01 + tx * f11;

				return (1 - ty) * f0 + ty * f1;
			}

			// valid for 0<k<1 and 0<x<1
			real_t CurveLUT::modCurve_x01_k01_RT(real_t x, real_t k)
			{
				real_t ret = 1 - math::pow(x, k);
				return math::pow(ret, 1 / k);
			}

			// extends range to support -1<x<0 and -1<k<0
			// outputs -1 to 1
			real_t CurveLUT::modCurve_xN11_kN11_RT(real_t x, real_t k)
			{
				static constexpr float CornerMargin = 0.6f; // .77 is quite sharp, 0.5 is mild and usable but maybe should be sharper
				k *= CornerMargin;
				k = clamp(k, -CornerMargin, CornerMargin);
				x = clampN11(x);
				if (k >= 0)
				{
					if (x > 0)
					{
						return 1 - modCurve_x01_k01_RT(x, 1 - k);
					}
					return modCurve_x01_k01_RT(-x, 1 - k) - 1;
				}
				if (x > 0)
				{
					return modCurve_x01_k01_RT(1 - x, 1 + k);
				}
				return -modCurve_x01_k01_RT(x + 1, 1 + k);
			}

			// incoming values should support -1,1 and output -1,1
			CurveLUT::CurveLUT(size_t nSamples)
				: LUT2D(nSamples, nSamples, [](float x01, float y01)
					{
						return modCurve_xN11_kN11_RT(x01 * 2 - 1, y01 * 2 - 1);
					})
			{}

			// user passes in n11 values; need to map N11 to 01
			float CurveLUT::Invoke(float xN11, float yN11) const {
				return LUT2D::Invoke(xN11 * .5f + .5f, yN11 * .5f + .5f);
			}


			/**
				* Converts a linear value to decibels.  Returns <= aMinDecibels if the linear
				* value is 0.
				*/
			float LinearToDecibels(float aLinearValue, float aMinDecibels)
			{
				return (aLinearValue > FloatEpsilon) ? 20.0f * math::log10(aLinearValue) : aMinDecibels;
			}

			/**
				* Converts a decibel value to a linear value.
				*/
			float DecibelsToLinear(float aDecibels, float aNegInfDecibels)
			{
				float lin = math::pow(10.0f, 0.05f * aDecibels);
				if (lin <= aNegInfDecibels)
					return 0.0f;
				return lin;
			}

			// don't use a LUT because we want to support pitch bend and glides and stuff. using a LUT + interpolation would be
			// asinine.
			float MIDINoteToFreq(float x)
			{
				// float a = 440;
				// return (a / 32.0f) * fast::pow(2.0f, (((float)x - 9.0f) / 12.0f));
				//return 440 * math::pow2((x - 69) / 12);
				static constexpr float oneTwelfth = 1.0f / 12;
				return 440 * math::pow2_N16_16((x - 69) * oneTwelfth);
			}

			float SemisToFrequencyMul(float x)
			{
				return math::pow2_N16_16(x / 12);
				//return math::pow2(x / 12.0f);
			}

			//float FrequencyToMIDINote(float hz)
			//{
			//	float ret = 12.0f * math::log2(max(8.0f, hz) / 440) + 69;
			//	return ret;
			//}

			FloatPair PanToLRVolumeParams(float panN11)
			{
				// -1..+1  -> 1..0
				panN11 = clampN11(panN11);
				float rightVol = (panN11 + 1) * 0.5f;
				return { 1 - rightVol, rightVol };
			}

			FloatPair PanToFactor(float panN11)
			{
				// SQRT pan law
				auto volumes = PanToLRVolumeParams(panN11);
				float leftChannel = math::sqrt01(volumes.first);
				float rightChannel = math::sqrt01(volumes.second);
				return { leftChannel, rightChannel };
			}

		} // namespace math

		void BipolarDistribute(size_t count, const bool* enabled, float* outp)
		{
			size_t enabledCount = 0;
			// get enabled count first
			for (size_t i = 0; i < count; ++i) {
				if (enabled[i]) enabledCount++;
			}
			size_t pairCount = enabledCount / 2; // chops off the odd one.
			size_t iPair = 0;
			size_t iEnabled = 0;
			for (size_t iElement = 0; iElement < count; ++iElement) {
				outp[iElement] = 0;// produce valid values even for disabled elements; they get used sometimes for optimization
				if (!enabled[iElement]) {
					continue;
				}
				if (iPair >= pairCount) {
					// past the expected # of pairs; this means it's centered. there should only ever be 1 of these elements (the odd one)
					++iEnabled;
					continue;
				}
				float val = float(iPair + 1) / pairCount; // +1 so 1. we don't use 0; that's reserved for the odd element, and 2. so we hit +1.0f
				if (iEnabled & 1) { // is this the 1st or 2nd in the pair
					val = -val;
					++iPair;
				}
				++iEnabled;
				outp[iElement] = val;
			}
		}

		//Float01Param::Float01Param(real_t& ref, real_t initialValue) : Float01RefParam(ref, initialValue) {}
		Float01Param::Float01Param(real_t& ref) : Float01RefParam(ref) {}
		float Float01Param::Get01Value(real_t modVal) const {
			return math::clamp01(mParamValue + modVal);
		}

		//FloatN11Param::FloatN11Param(real_t& ref, real_t initialValueN11) : Float01RefParam(ref, initialValueN11 * .5f + .5f) {
		//}
		FloatN11Param::FloatN11Param(real_t& ref) : Float01RefParam(ref) {
			CacheValue();
		}

		real_t FloatN11Param::GetN11Value(real_t modVal) const {
			real_t r = mParamValue + modVal;
			return math::clampN11(r * 2 - 1);
		}
		void FloatN11Param::SetN11Value(real_t v) {
			SetParamValue(v * .5f + .5f);
			CacheValue();
		}







		EnvTimeParam::EnvTimeParam(real_t& ref, real_t initialParamValue) : Float01RefParam(ref, initialParamValue) {}
		EnvTimeParam::EnvTimeParam(real_t& ref) : Float01RefParam(ref) {}

		real_t EnvTimeParam::GetMilliseconds(real_t paramModulation) const
		{
			float param = mParamValue + paramModulation; // apply current modulation value.
			param = math::clamp01(param);
			param -= 0.5f;       // -.5 to .5
			param *= gRangeLog2; // -5 to +5 (2^-5 = .0312; 2^5 = 32), with 375ms center val means [12ms, 12sec]
			float fact = math::pow2_N16_16(param);
			param = gCenterValue * fact;
			param -= gMinRawVal; // pow(2,x) doesn't ever reach 0 value. subtracting the min allows 0 to exist.
			return math::clamp(param, gMinRealVal, gMaxRealVal);
		}


		//IntParam::IntParam(real_t& ref, int minValueInclusive, int maxValueInclusive, int initialValue) :
		//    Float01Param(ref),
		//    mMinValueInclusive(minValueInclusive),
		//    mMaxValueInclusive(maxValueInclusive),
		//    mHalfMinusMinVal(0.5f - minValueInclusive)
		//{
		//    SetIntValue(initialValue);
		//}
		IntParam::IntParam(real_t& ref, const IntParamConfig& cfg) :
			Float01Param(ref),
			mCfg(cfg)
			//mMinValueInclusive(minValueInclusive),
			//mMaxValueInclusive(maxValueInclusive)//,
			//mHalfMinusMinVal(0.5f - minValueInclusive)
		{
			CacheValue();
		}

		//int IntParam::GetDiscreteValueCount() const {
		//	return mMaxValueInclusive - mMinValueInclusive + 1;
		//}
		// we want to split the float 0-1 range into equal portions. so if you have 3 values (0 - 2 inclusive),
		//     0      1        2
		// |------|-------|-------|
		// 0     .33     .66     1.00
		int IntParam::GetIntValue() const {
			int c = mCfg.GetDiscreteValueCount();
			int r = (int)(mParamValue * c);
			r = math::ClampI(r, 0, c - 1);
			return r + mCfg.mMinValInclusive;
		}
		void IntParam::SetIntValue(int val) {
			int c = mCfg.GetDiscreteValueCount();
			//if (val == this->mMinValueInclusive) {
			//	mParamValue = 0; // helps compression!
			//}
			//else if (val == this->mMaxValueInclusive) {
			//	mParamValue = 1; // helps compression!
			//} else {
				real_t p = real_t(val);// +0.5f; // so it lands in the middle of a bucket.
				p += 0.5f - mCfg.mMinValInclusive;
				p /= c;
				mParamValue = p;
			//}
			CacheValue();
		}

		ScaledRealParam::ScaledRealParam(real_t& ref, real_t minValueInclusive, real_t maxValueInclusive) :
			Float01Param(ref),
			mMinValueInclusive(minValueInclusive),
			mMaxValueInclusive(maxValueInclusive)
		{
			CacheValue();
		}

		real_t ScaledRealParam::GetRange() const {
			return mMaxValueInclusive - mMinValueInclusive;
		}

		real_t ScaledRealParam::GetRangedValue() const {
			real_t range = GetRange();
			real_t r = mParamValue * range;
			r = math::clamp(r, 0, range);
			return r + mMinValueInclusive;
		}

		void ScaledRealParam::SetRangedValue(real_t val) {
			mParamValue = (val - mMinValueInclusive) / GetRange();
			CacheValue();
		}



		float VolumeParam::ParamToLinear(float x) const
		{
			if (x <= 0)
				return 0;
			return x * x * mMaxVolumeLinearGain;
		}
		float VolumeParam::LinearToParam(float x) const
		{ // expensive
			return math::sqrt(x / mMaxVolumeLinearGain);
		}

		float VolumeParam::ParamToDecibels(float x) const
		{
			return math::LinearToDecibels(ParamToLinear(x));
		}
		float VolumeParam::DecibelsToParam(float db) const
		{ // pretty expensive
			return LinearToParam(math::DecibelsToLinear(db));
		}

		//VolumeParam::VolumeParam(real_t& ref, real_t maxDecibels, real_t initialParamValue01) :
		//    Float01Param(ref, initialParamValue01),
		//    mMaxVolumeLinearGain(math::DecibelsToLinear(maxDecibels))
		//{}
		VolumeParam::VolumeParam(real_t& ref, real_t maxDecibels) :
			Float01Param(ref),
			mMaxVolumeLinearGain(math::DecibelsToLinear(maxDecibels))
		{}

		float VolumeParam::GetLinearGain(float modVal) const
		{
			return ParamToLinear(mParamValue + modVal);
		}
		float VolumeParam::GetDecibels() const // expensive (ish)
		{
			return ParamToDecibels(mParamValue);
		}

		float VolumeParam::IsSilent() const
		{
			return math::IsSilentGain(GetLinearGain());
		}
		void VolumeParam::SetLinearValue(float f)
		{ // expensive
			mParamValue = LinearToParam(f);
		}
		void VolumeParam::SetDecibels(float db)
		{ // expensive
			mParamValue = DecibelsToParam(db);
		}


		//FrequencyParam::FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/, real_t initialValue, real_t initialKT) :
		//    mValue(valRef, initialValue),
		//    mKTValue(ktRef, initialKT),
		//    mCenterFrequency(centerFrequency),
		//    mCenterMidiNote(math::FrequencyToMIDINote(centerFrequency)),
		//    mScale(scale)
		//{}

		FrequencyParam::FrequencyParam(real_t& valRef, real_t& ktRef, const FreqParamConfig& cfg) :
			mValue(valRef),
			mKTValue(ktRef),
			mCfg(cfg)
		{
			//
		}

		//FrequencyParam::FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/) :
		//	mValue(valRef),
		//	mKTValue(ktRef),
		//	mCenterFrequency(centerFrequency),
		//	mCenterMidiNote(math::FrequencyToMIDINote(centerFrequency)),
		//	mScale(scale)
		//{}

		// noteHz is the playing note, to support key-tracking.
		float FrequencyParam::GetFrequency(float noteHz, float paramModulation) const
		{
			float param = mValue.Get01Value() + paramModulation; // apply current modulation value.
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
			float centerFreq = math::lerp(mCfg.mCenterFrequency, ktFreq, mKTValue.Get01Value());

			param -= 0.5f;  // signed distance from 0.5 -.2 (0.3 = -.2, 0.8 = .3)   [-.5,+.5]
			param *= mCfg.mScale;// 10.0f; // (.3 = -2, .8 = 3) [-15,+15]
			//float fact = math::pow(2, param);
			float fact = math::pow2_N16_16(param);
			return math::clamp(centerFreq * fact, 0.0f, 22050.0f);
		}

		void FrequencyParam::SetFrequencyAssumingNoKeytracking(float hz) {
			// 2 ^ param
			float  p = math::log2(hz);
			p /= mCfg.mScale;
			p += 0.5f;
			this->mValue.SetParamValue(math::clamp01(p));
		}

		// param modulation is normal krate param mod
		// noteModulation includes osc.mPitchFine + osc.mPitchSemis + detune;
		float FrequencyParam::GetMidiNote(float playingMidiNote, float paramModulation) const
		{
			float ktNote = playingMidiNote + 24; // center represents playing note + 2 octaves.

			float centerNote = math::lerp(mCfg.mCenterMidiNote, ktNote, mKTValue.Get01Value());

			float param = mValue.Get01Value() + paramModulation;

			param = (param - 0.5f) * mCfg.mScale;// 10; // rescale from 0-1 to -5 to +5 (octaves)
			float paramSemis =
				centerNote + param * 12; // each 1 param = 1 octave. because we're in semis land, it's just a mul.
			return paramSemis;
		}


		Serializer::~Serializer() {
			delete[] mBuffer;
		}

		Pair<uint8_t*, size_t> Serializer::DetachBuffer()
		{
			Pair<uint8_t*, size_t> ret{ mBuffer, mSize };
			mBuffer = nullptr;
			mAllocatedSize = mSize = 0;
			return ret;
		}

		uint8_t* Serializer::GrowBy(size_t n)
		{
			size_t newSize = mSize + n;
			if (mAllocatedSize < newSize) {
				size_t newAllocatedSize = newSize * 2 + 100;
				uint8_t* newBuffer = new uint8_t[newAllocatedSize];
				memcpy(newBuffer, mBuffer, mSize);
				delete[] mBuffer;
				mBuffer = newBuffer;
				mAllocatedSize = newAllocatedSize;
			}
			auto ret = mBuffer + mSize;
			mSize = newSize;
			return ret;
		}

		void Serializer::WriteUByte(uint8_t b) {
			*GrowBy(sizeof(b)) = b;
		}
		//    void WriteSByte(int8_t b) {
		//        auto p = (int8_t*)GrowBy(sizeof(b));
		//        *p = b;
		//    }
		//    void WriteUWord(uint16_t b) {
		//        auto p = (uint16_t*)GrowBy(sizeof(b));
		//        *p = b;
		//    }
		//    void WriteSWord(int16_t b) {
		//        auto p = (int16_t*)GrowBy(sizeof(b));
		//        *p = b;
		//    }
		void Serializer::WriteFloat(float f) {
			auto p = (float*)(GrowBy(sizeof(f)));
			*p = f;
		}
		void Serializer::WriteUInt32(uint32_t f) {
			auto p = (uint32_t*)(GrowBy(sizeof(f)));
			*p = f;
		}
		void Serializer::WriteBuffer(const uint8_t* buf, size_t n)
		{
			auto p = GrowBy(n);
			memcpy(p, buf, n);
		}


		Deserializer::Deserializer(const uint8_t* p) :
			mpData(p),
			mpCursor(p)//,
			//mSize(n),
			//mpEnd(p + n)
		{}
		//int8_t ReadSByte() {
		//    int8_t ret = *((int8_t*)mpCursor);
		//    mpCursor += sizeof(ret);
		//    return ret;
		//}
		uint8_t Deserializer::ReadUByte() {
			uint8_t ret = *((uint8_t*)mpCursor);
			mpCursor += sizeof(ret);
			return ret;
		}

		//int16_t ReadSWord() {
		//    int16_t ret = *((int16_t*)mpCursor);
		//    mpCursor += sizeof(ret);
		//    return ret;
		//}
		//uint16_t ReadUWord() {
		//    uint16_t ret = *((uint16_t*)mpCursor);
		//    mpCursor += sizeof(ret);
		//    return ret;
		//}

		uint32_t Deserializer::ReadUInt32() {
			uint32_t ret = *((uint32_t*)mpCursor);
			mpCursor += sizeof(ret);
			return ret;
		}

		float Deserializer::ReadFloat() {
			float ret = *((float*)mpCursor);
			mpCursor += sizeof(ret);
			return ret;
		}

		double Deserializer::ReadDouble() {
			double ret = *((double*)mpCursor);
			mpCursor += sizeof(ret);
			return ret;
		}

		// returns a new cursor in the out buffer 
		void Deserializer::ReadBuffer(void* out, size_t numbytes) {
			memcpy(out, mpCursor, numbytes);
			mpCursor += numbytes;
		}

		//ByteBitfield ReadByteBitfield() {
		//    return ByteBitfield{ ReadUByte() };
		//}
		//WordBitfield ReadWordBitfield() {
		//    return WordBitfield{ ReadUWord() };
		//}


	} // namespace M7


} // namespace WaveSabreCore








