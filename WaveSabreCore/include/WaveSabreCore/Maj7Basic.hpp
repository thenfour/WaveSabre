#pragma once

#include <WaveSabreCore/Helpers.h>
#include <algorithm>
#include <memory>

// correction for windows.h macros.
// i cannot use NOMINMAX, because VST and Gdiplus depend on these macros. 
#undef min
#undef max
using std::min;
using std::max;

namespace WaveSabreCore
{
    namespace M7
    {
        using real_t = float;

        static constexpr real_t FloatEpsilon = 0.000001f;

        namespace fastmath
        {

            static inline float fasterlog2(float x)
            {
                union {
                    float f;
                    uint32_t i;
                } vx = { x };
                float y = (float)vx.i;
                y *= 1.1920928955078125e-7f;
                return y - 126.94269504f;
            }
            static inline float fasterlog(float x)
            {
                //  return 0.69314718f * fasterlog2 (x);

                union {
                    float f;
                    uint32_t i;
                } vx = { x };
                float y = (float)vx.i;
                y *= 8.2629582881927490e-8f;
                return y - 87.989971088f;
            }

        }

        // we may be able to save code size by calling exported functions instead of pulling in lib fns
        namespace math
        {
            static constexpr real_t gPI = 3.1415926535897932385f;

            inline real_t pow(real_t x, real_t y) {
                return ::powf(x, y);
            }
            inline real_t log2(real_t x) {
                return fastmath::fasterlog2(x);
            }
            inline real_t log10(real_t x) {
                return fastmath::fasterlog(x);
            }
            
            inline real_t sqrt(real_t x) {
                return ::sqrtf(x);
            }
            inline real_t floor(real_t x) {
                return (real_t)::floor((double)x);
            }
            inline real_t sin(real_t x) {
                return (real_t)Helpers::FastSin((double)x);
            }
            inline real_t tan(real_t x) {
                return (real_t)::tan((double)x);
            }
            inline real_t max(real_t x, real_t y) {
                return x > y ? x : y;
            }
            inline real_t min(real_t x, real_t y) {
                return x < y ? x : y;
            }
            inline real_t tanh(real_t x) {
                return (real_t)::tanh((double)x);
            }
            inline real_t tanh(real_t x, real_t y) { // used by saturation
                return (real_t)::tanh((double)(x * y));
            }
        }

        inline bool FloatEquals(real_t f1, real_t f2, real_t eps = FloatEpsilon)
        {
            return ::fabsf(f1 - f2) < eps;
        }

        inline bool FloatLessThanOrEquals(real_t lhs, real_t rhs, real_t eps = FloatEpsilon)
        {
            return lhs <= (rhs + eps);
        }

        inline bool FloatIsAbove(real_t lhs, real_t rhs, real_t eps = FloatEpsilon)
        {
            return lhs > (rhs + eps);
        }

        inline real_t Clamp(real_t x, real_t low, real_t hi)
        {
            if (x <= low)
                return low;
            if (x >= hi)
                return hi;
            return x;
        }

        // for floating point types, it's helpful to be clear that the value is inclusive.
        template <typename T>
        static T ClampInclusive(T x, T minInclusive, T maxInclusive)
        {
            if (x <= minInclusive)
                return minInclusive;
            if (x >= maxInclusive)
                return maxInclusive;
            return x;
        }

        template <typename T>
        static T ClampI(T x, T minInclusive, T maxInclusive)
        {
            if (x <= minInclusive)
                return minInclusive;
            if (x >= maxInclusive)
                return maxInclusive;
            return x;
        }

        static float Lerp(float a, float b, float t)
        {
            return a * (1.0f - t) + b * t;
        }

        inline real_t CalculateInc01PerSampleForMS(real_t ms)
        {
            return Clamp(1000.0f / (math::max(0.01f, ms) * (real_t)Helpers::CurrentSampleRate), 0, 1);
        }

        // valid for 0<k<1 and 0<x<1
        inline real_t modCurve_x01_k01(real_t x, real_t k)
        {
            real_t ret = 1 - math::pow(x, k);
            return math::pow(ret, 1 / k);
        }

        // extends range to support -1<x<0 and -1<k<0
        // outputs -1 to 1
        inline real_t modCurve_xN11_kN11(real_t x, real_t k)
        {
            static constexpr real_t CornerMargin = real_t(0.77);
            k *= CornerMargin;
            k = Clamp(k, -CornerMargin, CornerMargin);
            x = Clamp(x, -1, 1);
            if (k >= 0)
            {
                if (x > 0)
                {
                    return 1 - modCurve_x01_k01(x, 1 - k);
                }
                return modCurve_x01_k01(-x, 1 - k) - 1;
            }
            if (x > 0)
            {
                return modCurve_x01_k01(1 - x, 1 + k);
            }
            return -modCurve_x01_k01(x + 1, 1 + k);
        }

        static constexpr float MIN_DECIBEL_GAIN = -60.0f;

        /**
         * Converts a linear value to decibels.  Returns <= aMinDecibels if the linear
         * value is 0.
         */
        inline float LinearToDecibels(float aLinearValue, float aMinDecibels = MIN_DECIBEL_GAIN)
        {
            return (aLinearValue > FloatEpsilon) ? 20.0f * log10f(aLinearValue) : aMinDecibels;
        }

        /**
         * Converts a decibel value to a linear value.
         */
        inline float DecibelsToLinear(float aDecibels, float aNegInfDecibels = MIN_DECIBEL_GAIN)
        {
            float lin = math::pow(10.0f, 0.05f * aDecibels);
            if (lin <= aNegInfDecibels)
                return 0.0f;
            return lin;
        }

        static const float gMinGainLinear = DecibelsToLinear(MIN_DECIBEL_GAIN);

        inline bool IsSilentGain(float gain)
        {
            return gain <= gMinGainLinear;
        }



        // don't use a LUT because we want to support pitch bend and glides and stuff. using a LUT + interpolation would be
        // asinine.
        inline float MIDINoteToFreq(float x)
        {
            // float a = 440;
            // return (a / 32.0f) * fast::pow(2.0f, (((float)x - 9.0f) / 12.0f));
            return 440 * math::pow(2.0f, ((x - 69) / 12));
        }

        // let MidiNoteToFrequency = function(midiNote) {
        //   return 440 * Math.pow(2, (midiNote - 69) / 12);
        // };
        // let FrequencyToMidiNote = (hz) => {
        //   return 12.0 * Math.log2(Math.max(8, hz) / 440) + 69;
        // };

        inline float FrequencyToMIDINote(float hz)
        {
            float ret = 12.0f * math::log2(max(8.0f, hz) / 440) + 69;
            return ret;
        }


        // raw param values are stored in a huge array. the various sub-objects (osc, filters, envelopes)
        // will refer to param values in that array using this. sub classes of this class can do whatever
        // needed alterations mappings etc.
        struct Float01RefParam
        {
        protected:
            real_t& mParamValue;

        public:
            explicit Float01RefParam(real_t& ref) : mParamValue(ref) {}
            explicit Float01RefParam(real_t& ref, real_t initialValue) : mParamValue(ref) {
                mParamValue = initialValue;
            }

            real_t GetRawParamValue() const {
                return mParamValue;
            }
            // you should rarely need to set this outside of subclasses because params are set using the backed float value.
            // but one case is when you need to do an ad-hoc conversion.
            void SetParamValue(real_t v) {
                mParamValue = v;
            }
        };

        // overkill? maybe but no judging plz
        struct Float01Param : Float01RefParam
        {
            explicit Float01Param(real_t& ref, real_t initialValue = 0) : Float01RefParam(ref, initialValue) {}
            real_t Get01Value(real_t modVal = 0.0f) const {
                return mParamValue + modVal;
            }
        };

        struct FloatN11Param : Float01RefParam
        {
            explicit FloatN11Param(real_t& ref, real_t initialValue) : Float01RefParam(ref, initialValue) {

            }

            real_t GetN11Value(real_t modVal = 0.0f) const {
                real_t r = mParamValue + modVal;
                return Clamp(r * 2 - 1, -1, 1);
            }
            void SetN11Value(real_t v) {
                SetParamValue(v*.5f+.5f);
            }
        };

        // todo: time should not always be the same; allow configuring the range.
        // min value is always 0 milliseconds.
        struct EnvTimeParam : Float01RefParam
        {
            explicit EnvTimeParam(real_t& ref, real_t initialParamValue) : Float01RefParam(ref, initialParamValue) {}

            static constexpr real_t gCenterValue = 375; // the MS at 0.5 param value.
            static constexpr int gRangeLog2 = 10;      // must be even for below calculations to work
            static constexpr real_t gMinRawVal =
                (1.0f / (1 << (gRangeLog2 / 2))) *
                gCenterValue; // minimum value as described by log calc (before subtracting min).
            static constexpr real_t gMaxRawVal = (1 << (gRangeLog2 / 2)) * gCenterValue;
            static constexpr real_t gMinRealVal = 0;
            static constexpr real_t gMaxRealVal = gMaxRawVal - gMinRawVal;

            real_t GetMilliseconds(real_t paramModulation = 0.0f) const
            {
                float param = mParamValue + paramModulation; // apply current modulation value.
                param = Clamp(param, 0, 1);
                param -= 0.5f;       // -.5 to .5
                param *= gRangeLog2; // -5 to +5 (2^-5 = .0312; 2^5 = 32), with 375ms center val means [12ms, 12sec]
                float fact = math::pow(2, param);
                param = gCenterValue * fact;
                param -= gMinRawVal; // pow(2,x) doesn't ever reach 0 value. subtracting the min allows 0 to exist.
                return Clamp(param, gMinRealVal, gMaxRealVal);
            }

        };

        // paramvalue N11
        struct CurveParam : FloatN11Param
        {
            explicit CurveParam(real_t& ref, real_t initialParamValue) : FloatN11Param(ref, initialParamValue) {}

            real_t ApplyToValue(real_t x, real_t modVal = 0.0f) const {
                return modCurve_xN11_kN11(x, GetN11Value() + modVal);
            }
        };

        // stores an integral parameter as a 0-1 float param
        struct IntParam : Float01Param
        {
        protected:
            int mMinValueInclusive;
            int mMaxValueInclusive;
        public:
            explicit IntParam(real_t& ref, int minValueInclusive, int maxValueInclusive, int initialValue) :
                Float01Param(ref),
                mMinValueInclusive(minValueInclusive),
                mMaxValueInclusive(maxValueInclusive)
            {
                SetIntValue(initialValue);
            }

            int GetDiscreteValueCount() const {
                return mMaxValueInclusive - mMinValueInclusive + 1;
            }
            // we want to split the float 0-1 range into equal portions. so if you have 3 values (0 - 2 inclusive),
            //     0      1        2
            // |------|-------|-------|
            // 0     .33     .66     1.00
            int GetIntValue() const {
                int c = GetDiscreteValueCount();
                int r = (int)(mParamValue * c);
                r = ClampI(r, 0, c - 1);
                return r + mMinValueInclusive;
            }
            void SetIntValue(int val) {
                int c = GetDiscreteValueCount();
                real_t p = real_t(val);
                p -= mMinValueInclusive;
                p /= c;
                mParamValue = p;
            }
        };

        // relies on assumptions about the enum:
        template <typename T>
        struct EnumParam : IntParam
        {
            explicit EnumParam(real_t& ref, T maxValue, T initialValue) : IntParam(ref, 0, (int)maxValue, (int)initialValue)
            {}
            T GetEnumValue() const {
                return (T)GetIntValue();
            }
            void SetEnumValue(T v) {
                SetIntValue((int)v);
            }
        };

        struct BoolParam : IntParam
        {
            explicit BoolParam(real_t& ref, bool initialValue = false) : IntParam(ref, 0, 1, initialValue ? 1 : 0)
            {}

            bool GetBoolValue() const {
                return mParamValue > 0.5f;
            }
            void SetBoolValue(bool b) {
                mParamValue = b ? 0.0f : 1.0f;
            }
        };

        // stores a floating-point value which is always scaled linear to 0-1 param range.
        // this is useful because it
        // 1) attaches range info directly to the value
        // 2) ensures the param scale is always the same, more ergonomic for GUI.
        // 3) includes clamping
        // used for things like FrequencyMultiplier
        struct ScaledRealParam : Float01Param
        {
        protected:
            real_t mMinValueInclusive;
            real_t mMaxValueInclusive;

        public:
            explicit ScaledRealParam(real_t& ref, real_t minValueInclusive, real_t maxValueInclusive, real_t initialRangedValue) :
                Float01Param(ref),
                mMinValueInclusive(minValueInclusive),
                mMaxValueInclusive(maxValueInclusive)
            {
                SetRangedValue(initialRangedValue);
            }

            real_t GetRange() const {
                return mMaxValueInclusive - mMinValueInclusive;
            }

            real_t GetRangedValue() const {
                real_t range = GetRange();
                real_t r = mParamValue * range;
                r = Clamp(r, 0, range);
                return r + mMinValueInclusive;
            }

            void SetRangedValue(real_t val) {
                mParamValue = (val - mMinValueInclusive) / GetRange();
            }
        };

        // stores a volume parameter as a 0-1 float param with linear sweeping
        // "volume" parameters use a custom scale, so they feel more usable.
        // massive uses a square curve, and a -inf db to +1 db range.
        // various params use different volume ranges. master volume can be amplified, but oscillator volume should never be >1.
        // min volume is ALWAYS silence.
        struct VolumeParam : Float01Param
        {
        private:
            const float mMaxVolumeLinearGain;// 1.122f = +1 db.

            inline float ParamToLinear(float x) const
            {
                if (x <= 0)
                    return 0;
                return x * x * mMaxVolumeLinearGain;
            }
            inline float LinearToParam(float x) const
            { // expensive
                return math::sqrt(x / mMaxVolumeLinearGain);
            }

            inline float ParamToDecibels(float x) const
            {
                return LinearToDecibels(ParamToLinear(x));
            }
            inline float DecibelsToParam(float db) const
            { // pretty expensive
                return LinearToParam(DecibelsToLinear(db));
            }

        public:
            explicit VolumeParam(real_t& ref, real_t maxDecibels, real_t initialParamValue01) :
                Float01Param(ref, initialParamValue01),
                mMaxVolumeLinearGain(DecibelsToLinear(maxDecibels))
            {}

            float GetLinearGain(float modVal = 0.0f) const
            {
                return ParamToLinear(mParamValue + modVal);
            }
            float GetDecibels() const // expensive (ish)
            {
                return ParamToDecibels(mParamValue);
            }

            float IsSilent() const
            {
                return IsSilentGain(GetLinearGain());
            }
            void SetLinearValue(float f)
            { // expensive
                mParamValue = LinearToParam(f);
            }
            void SetDecibels(float db)
            { // expensive
                mParamValue = DecibelsToParam(db);
            }

            //// useful for adding a modulation signal to a volume.
            //VolumeParam AddParam(float rhs) const
            //{
            //    return FromParamValue(mParamValue + rhs);
            //}

            //static VolumeParam FromLinear(float f)
            //{
            //    VolumeParam ret;
            //    ret.SetLinearValue(f);
            //    return ret;
            //}
            //static VolumeParam FromParamValue(float f)
            //{
            //    VolumeParam ret;
            //    ret.mParamValue = f;
            //    return ret;
            //}
            //static VolumeParam FromDecibels(float f)
            //{
            //    VolumeParam ret;
            //    ret.SetDecibels(f);
            //    return ret;
            //}
        };

        // value 0.3 = unity, and each 0.1 param value = 1 octave transposition, when KT = 1.
        // when KT = 0, 0.5 = 1khz, and each 0.1 param value = +/- octave.
        struct FrequencyParam
        {
            const float mCenterMidiNote;
            const float mCenterFrequency;

        public:
            Float01Param mValue;
            Float01Param mKTValue; // how much key tracking to apply. when 0, the frequency doesn't change based on playing note. when 1, it scales completely with the note's frequency.

            explicit FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t initialValue, real_t initialKT) :
                mValue(valRef, initialValue),
                mKTValue(ktRef, initialKT),
                mCenterFrequency(centerFrequency),
                mCenterMidiNote(FrequencyToMIDINote(centerFrequency))
            {}

            // noteHz is the playing note, to support key-tracking.
            float GetFrequency(float noteHz, float paramModulation) const
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
                float ktFreq = noteHz * 4; // to copy massive, 1:1 is at paramvalue 0.3. 0.5 is 2 octaves above playing freq.
                float centerFreq = Lerp(mCenterFrequency, ktFreq, mKTValue.Get01Value());

                param -= 0.5f;  // signed distance from 0.5 -.2 (0.3 = -.2, 0.8 = .3)
                param *= 10.0f; // (.3 = -2, .8 = 3)
                float fact = math::pow(2, param);
                return Clamp(centerFreq * fact, 0.0f, 22050.0f);
            }

            // param modulation is normal krate param mod
            // noteModulation includes osc.mPitchFine + osc.mPitchSemis + detune;
            float GetMidiNote(float playingMidiNote, float paramModulation) const
            {
                //constexpr float oneKhzMidiNote =
                //    83.213094853f;                   // 1000hz, in midi notes. this replicates behavior of filter modulation.
                float ktNote = playingMidiNote + 24; // center represents playing note + 2 octaves.

                float centerNote = Lerp(mCenterMidiNote, ktNote, mKTValue.Get01Value());

                float param = mValue.Get01Value() + paramModulation;

                param = (param - 0.5f) * 10; // rescale from 0-1 to -5 to +5 (octaves)
                float paramSemis =
                    centerNote + param * 12; // each 1 param = 1 octave. because we're in semis land, it's just a mul.
                return paramSemis;
            }
        };

        // you need to sync up all these enums below to correspond with ordering et al.
        enum class ParamIndices
        {
            MasterVolume,
            VoicingMode,
            Unisono,
            UnisonoDetune,
            UnisonoStereoSpread,
            OscillatorDetune,
            OscillatorSpread,
            FMBrightness,

            Macro1,
            Macro2,
            Macro3,
            Macro4,

            PortamentoTime,
            PortamentoCurve,
            PitchBendRange,

            Osc1Enabled, // KEEP IN SYNC WITH OscParamIndexOffsets
            Osc1Volume,
            Osc1Waveform,
            Osc1Waveshape,
            Osc1PhaseRestart,
            Osc1PhaseOffset,
            Osc1SyncEnable,
            Osc1SyncFrequency,
            Osc1SyncFrequencyKT,
            Osc1FrequencyParam,
            Osc1FrequencyParamKT,
            Osc1PitchSemis,
            Osc1PitchFine,
            Osc1FreqMul,
            Osc1FMFeedback,

            Osc2Enabled, // KEEP IN SYNC WITH OscParamIndexOffsets
            Osc2Volume,
            Osc2Waveform,
            Osc2Waveshape,
            Osc2PhaseRestart,
            Osc2PhaseOffset,
            Osc2SyncEnable,
            Osc2SyncFrequency,
            Osc2SyncFrequencyKT,
            Osc2FrequencyParam,
            Osc2FrequencyParamKT,
            Osc2PitchSemis,
            Osc2PitchFine,
            Osc2FreqMul,
            Osc2FMFeedback,

            Osc3Enabled, // KEEP IN SYNC WITH OscParamIndexOffsets
            Osc3Volume,
            Osc3Waveform,
            Osc3Waveshape,
            Osc3PhaseRestart,
            Osc3PhaseOffset,
            Osc3SyncEnable,
            Osc3SyncFrequency,
            Osc3SyncFrequencyKT,
            Osc3FrequencyParam,
            Osc3FrequencyParamKT,
            Osc3PitchSemis,
            Osc3PitchFine,
            Osc3FreqMul,
            Osc3FMFeedback,

            AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
            AmpEnvAttackTime,
            AmpEnvAttackCurve,
            AmpEnvHoldTime,
            AmpEnvDecayTime,
            AmpEnvDecayCurve,
            AmpEnvSustainLevel,
            AmpEnvReleaseTime,
            AmpEnvReleaseCurve,
            AmpEnvLegatoRestart,

            Env1DelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
            Env1AttackTime,
            Env1AttackCurve,
            Env1HoldTime,
            Env1DecayTime,
            Env1DecayCurve,
            Env1SustainLevel,
            Env1ReleaseTime,
            Env1ReleaseCurve,
            Env1LegatoRestart,

            Env2DelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
            Env2AttackTime,
            Env2AttackCurve,
            Env2HoldTime,
            Env2DecayTime,
            Env2DecayCurve,
            Env2SustainLevel,
            Env2ReleaseTime,
            Env2ReleaseCurve,
            Env2LegatoRestart,

            LFO1Waveform, // KEEP IN SYNC WITH LFOParamIndexOffsets
            LFO1Waveshape,
            LFO1Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
            LFO1PhaseOffset,
            LFO1FrequencyParam,

            LFO2Waveform, // KEEP IN SYNC WITH LFOParamIndexOffsets
            LFO2Waveshape,
            LFO2Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
            LFO2PhaseOffset,
            LFO2FrequencyParam,

            FilterType,
            FilterQ,
            FilterSaturation,
            FilterFrequency,
            FilterFrequencyKT,

            FMAmt1to2,
            FMAmt1to3,
            FMAmt2to1,
            FMAmt2to3,
            FMAmt3to1,
            FMAmt3to2,

            Mod1Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod1Source,
            Mod1Destination,
            Mod1Curve,
            Mod1Scale,

            Mod2Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod2Source,
            Mod2Destination,
            Mod2Curve,
            Mod2Scale,

            Mod3Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod3Source,
            Mod3Destination,
            Mod3Curve,
            Mod3Scale,

            Mod4Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod4Source,
            Mod4Destination,
            Mod4Curve,
            Mod4Scale,

            Mod5Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod5Source,
            Mod5Destination,
            Mod5Curve,
            Mod5Scale,

            Mod6Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod6Source,
            Mod6Destination,
            Mod6Curve,
            Mod6Scale,

            Mod7Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod7Source,
            Mod7Destination,
            Mod7Curve,
            Mod7Scale,

            Mod8Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod8Source,
            Mod8Destination,
            Mod8Curve,
            Mod8Scale,

            NumParams,
        };

        // 
        #define MAJ7_PARAM_VST_NAMES { \
		    {"Master"}, \
		    {"PolyMon"}, \
		    {"Unisono"}, \
		    {"UniDet"}, \
		    {"UniSpr"}, \
		    {"OscDet"}, \
		    {"OscSpr"}, \
		    {"FMBrigh"}, \
		    {"Macro1"}, \
		    {"Macro2"}, \
		    {"Macro3"}, \
		    {"Macro4"}, \
		    {"PortTm"}, \
		    {"PortCv"}, \
		    {"PBRng"}, \
		    {"O1En"}, \
		    {"O1Vol"}, \
		    {"O1Wave"}, \
		    {"O1Shp"}, \
		    {"O1PRst"}, \
		    {"O1Poff"}, \
		    {"O1Scen"}, \
		    {"O1ScFq"}, \
		    {"O1ScKt"}, \
		    {"O1Fq"}, \
		    {"O1FqKt"}, \
		    {"O1Semi"}, \
		    {"O1Fine"}, \
		    {"O1Mul"}, \
		    {"O1FMFb"}, \
		    {"O2En"}, \
		    {"O2Vol"}, \
		    {"O2Wave"}, \
		    {"O2Shp"}, \
		    {"O2PRst"}, \
		    {"O2Poff"}, \
		    {"O2Scen"}, \
		    {"O2ScFq"}, \
		    {"O2ScKt"}, \
		    {"O2Fq"}, \
		    {"O2FqKt"}, \
		    {"O2Semi"}, \
		    {"O2Fine"}, \
		    {"O2Mul"}, \
		    {"O2FMFb"}, \
		    {"O3En"}, \
		    {"O3Vol"}, \
		    {"O3Wave"}, \
		    {"O3Shp"}, \
		    {"O3PRst"}, \
		    {"O3Poff"}, \
		    {"O3Scen"}, \
		    {"O3ScFq"}, \
		    {"O3ScKt"}, \
		    {"O3Fq"}, \
		    {"O3FqKt"}, \
		    {"O3Semi"}, \
		    {"O3Fine"}, \
		    {"O3Mul"}, \
		    {"O3FMFb"}, \
		    {"AEdlt"}, \
		    {"AEatt"}, \
		    {"AEatc"}, \
		    {"AEht"}, \
		    {"AEdt"}, \
		    {"AEdc"}, \
		    {"AEsl"}, \
		    {"AErt"}, \
		    {"AEtc"}, \
		    {"AErst"}, \
		    {"E1dlt"}, \
		    {"E1att"}, \
		    {"E1atc"}, \
		    {"E1ht"}, \
		    {"E1dt"}, \
		    {"E1dc"}, \
		    {"E1sl"}, \
		    {"E1rt"}, \
		    {"E1tc"}, \
		    {"E1rst"}, \
		    {"E2dlt"}, \
		    {"E2att"}, \
		    {"E2atc"}, \
		    {"E2ht"}, \
		    {"E2dt"}, \
		    {"E2dc"}, \
		    {"E2sl"}, \
		    {"E2rt"}, \
		    {"E2tc"}, \
		    {"E2rst"}, \
		    {"LFO1wav"}, \
		    {"LFO1shp"}, \
		    {"LFO1rst"}, \
		    {"LFO1ph"}, \
		    {"LFO1fr"}, \
		    {"LFO2wav"}, \
		    {"LFO2shp"}, \
		    {"LFO2rst"}, \
		    {"LFO2ph"}, \
		    {"LFO2fr"}, \
		    {"FLtype"}, \
		    {"FLq"}, \
		    {"FLsat"}, \
		    {"FLfreq"}, \
		    {"FLkt"}, \
		    {"FM1to2"}, \
		    {"FM1to3"}, \
		    {"FM2to1"}, \
		    {"FM2to3"}, \
		    {"FM3to1"}, \
		    {"FM3to2"}, \
		    {"M1en"}, \
		    {"M1src"}, \
		    {"M1dest"}, \
		    {"M1curv"}, \
		    {"M1scale"}, \
		    {"M2en"}, \
		    {"M2src"}, \
		    {"M2dest"}, \
		    {"M2curv"}, \
		    {"M2scale"}, \
		    {"M3en"}, \
		    {"M3src"}, \
		    {"M3dest"}, \
		    {"M3curv"}, \
		    {"M3scale"}, \
		    {"M4en"}, \
		    {"M4src"}, \
		    {"M4dest"}, \
		    {"M4curv"}, \
		    {"M4scale"}, \
		    {"M5en"}, \
		    {"M5src"}, \
		    {"M5dest"}, \
		    {"M5curv"}, \
		    {"M5scale"}, \
		    {"M6en"}, \
		    {"M6src"}, \
		    {"M6dest"}, \
		    {"M6curv"}, \
		    {"M6scale"}, \
		    {"M7en"}, \
		    {"M7src"}, \
		    {"M7dest"}, \
		    {"M7curv"}, \
		    {"M7scale"}, \
		    {"M8en"}, \
		    {"M8src"}, \
		    {"M8dest"}, \
		    {"M8curv"}, \
		    {"M8scale"}, \
        }

        enum class ModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Enabled,
            Source,
            Destination,
            Curve,
            Scale,
        };
        enum class LFOParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Waveform,
            Waveshape,
            Restart,
            PhaseOffset,
            FrequencyParam,
        };
        enum class EnvParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            DelayTime,
            AttackTime,
            AttackCurve,
            HoldTime,
            DecayTime,
            DecayCurve,
            SustainLevel,
            ReleaseTime,
            ReleaseCurve,
            LegatoRestart,
        };
        enum class OscParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Enabled,
            Volume,
            Waveform,
            Waveshape,
            PhaseRestart,
            PhaseOffset,
            SyncEnable,
            SyncFrequency,
            SyncFrequencyKT,
            FrequencyParam,
            FrequencyParamKT,
            PitchSemis,
            PitchFine,
            FreqMul,
            FMFeedback,
        };

    } // namespace M7


} // namespace WaveSabreCore








