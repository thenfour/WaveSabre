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

#ifdef __cplusplus
#define cast_uint32_t static_cast<uint32_t>
#else
#define cast_uint32_t (uint32_t)
#endif

namespace WaveSabreCore
{
    namespace M7
    {
        using real_t = float;

        static constexpr real_t FloatEpsilon = 0.000001f;

        namespace fastmath
        {

            static inline float fastpow2(float p)
            {
                float offset = (p < 0) ? 1.0f : 0.0f;
                float clipp = (p < -126) ? -126.0f : p;
                int w = (int)clipp;
                float z = clipp - w + offset;
                union {
                    uint32_t i;
                    float f;
                } v = { cast_uint32_t((1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z)) };

                return v.f;
            }

            static inline float fastlog2(float x)
            {
                union {
                    float f;
                    uint32_t i;
                } vx = { x };
                union {
                    uint32_t i;
                    float f;
                } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
                float y = (float)vx.i;
                y *= 1.1920928955078125e-7f;

                return y - 124.22551499f - 1.498030302f * mx.f - 1.72587999f / (0.3520887068f + mx.f);
            }

            static inline float fastpow(float x, float p)
            {
                return fastpow2(p * fastlog2(x));
            }

            static inline float fasterpow2(float p)
            {
                float clipp = (p < -126) ? -126.0f : p;
                union {
                    uint32_t i;
                    float f;
                } v = { cast_uint32_t((1 << 23) * (clipp + 126.94269504f)) };
                return v.f;
            }

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

            static inline float fasterpow(float x, float p)
            {
                return fasterpow2(p * fasterlog2(x));
            }

            static inline float fasterexp(float p)
            {
                return fasterpow2(1.442695040f * p);
            }

            static inline float fastertanh(float p)
            {
                return -1.0f + 2.0f / (1.0f + fasterexp(-2.0f * p));
            }

            static inline float fastersin(float x)
            {
                return (float)Helpers::FastSin((double)x);
                //static const float fouroverpi = 1.2732395447351627f;
                //static const float fouroverpisq = 0.40528473456935109f;
                //static const float q = 0.77633023248007499f;
                //union {
                //    float f;
                //    uint32_t i;
                //} p = { 0.22308510060189463f };

                //union {
                //    float f;
                //    uint32_t i;
                //} vx = { x };
                //uint32_t sign = vx.i & 0x80000000;
                //vx.i &= 0x7FFFFFFF;

                //float qpprox = fouroverpi * x - fouroverpisq * x * vx.f;

                //p.i |= sign;

                //return qpprox * (q + p.f * qpprox);
            }

            static inline float fastercos(float x)
            {
                return (float)Helpers::FastCos((double)x);
                //static const float twooverpi = 0.63661977236758134f;
                //static const float p = 0.54641335845679634f;

                //union {
                //    float f;
                //    uint32_t i;
                //} vx = { x };
                //vx.i &= 0x7FFFFFFF;

                //float qpprox = 1.0f - twooverpi * vx.f;

                //return qpprox + p * qpprox * (1.0f - qpprox * qpprox);
            }

            static inline float fastertanfull(float x)
            {
                static const float twopi = 6.2831853071795865f;
                static const float invtwopi = 0.15915494309189534f;

                int k = (int)(x * invtwopi);
                float half = (x < 0) ? -0.5f : 0.5f;
                float xnew = x - (half + k) * twopi;

                return fastersin(xnew) / fastercos(xnew);
            }

        }

        // we may be able to save code size by calling exported functions instead of pulling in lib fns
        namespace math
        {
            static constexpr real_t gPI = 3.14159265358979323846264338327950288f;
            static constexpr real_t gPITimes2 = gPI * 2;

            inline real_t pow(real_t x, real_t y) {
                // fasterpow is just too inaccurate.
                //return fastmath::fastpow(x, y); // this is also not always accurate, just due to ranges. 
                return ::powf(x, y);
            }
            inline real_t pow2(real_t y) {
                return fastmath::fastpow2(y);
            }
            inline real_t log2(real_t x) {
                return fastmath::fasterlog2(x);
            }
            inline real_t log10(real_t x) {
                return fastmath::fasterlog(x);
            }
            
            inline real_t sqrt(real_t x) {
                return ::sqrtf(x); // TODO: is fasterpow(x, 0.5) faster?
            }
            inline real_t floor(real_t x) {
                return (real_t)::floor((double)x);
            }
            inline real_t sin(real_t x) {
                return (real_t)Helpers::FastSin((double)x);
            }
            inline real_t cos(real_t x) {
                return (real_t)Helpers::FastCos((double)x);
            }
            inline real_t tan(real_t x) {
                return fastmath::fastertanfull(x);
            }
            inline real_t max(real_t x, real_t y) {
                return x > y ? x : y;
            }
            inline real_t min(real_t x, real_t y) {
                return x < y ? x : y;
            }
            inline real_t tanh(real_t x) {
                return fastmath::fastertanh(x);
            }
            inline real_t tanh(real_t x, real_t y) { // used by saturation
                return tanh(x * y);
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
            static constexpr float CornerMargin = 0.5f; // real_t(0.77);
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

        inline float MillisecondsToSamples(float ms)
        {
            static constexpr float oneOver1000 = 1.0f / 1000.0f; // obsessive optimization?
            return (ms * Helpers::CurrentSampleRateF) * oneOver1000;
        }


        // don't use a LUT because we want to support pitch bend and glides and stuff. using a LUT + interpolation would be
        // asinine.
        inline float MIDINoteToFreq(float x)
        {
            // float a = 440;
            // return (a / 32.0f) * fast::pow(2.0f, (((float)x - 9.0f) / 12.0f));
            return 440 * math::pow2((x - 69) / 12);
        }

        inline float SemisToFrequencyMul(float x)
        {
            return math::pow2(x / 12.0f);
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

        inline std::pair<float, float> PanToFactor(float panN11)
        {
            // SQRT pan law
            // -1..+1  -> 1..0
            float normPan = (-panN11 + 1) / 2;
            float leftChannel = math::sqrt(normPan);
            float rightChannel = math::sqrt(1.0f - normPan);
            return std::make_pair(leftChannel, rightChannel);
        }

        inline float Fract(float x) {
            return x - math::floor(x);
        }

        // where t1, t2, and x are periodic values [0,1).
        // and t1 is "before" t2,
        // return true if x falls between t1 and t2.
        // so if t2 < t1, it means a cycle wrap.
        inline bool DoesEncounter(float t1, float t2, float x) {
            if (t1 < t2) {
                return (t1 < x&& x <= t2);
            }
            return (x <= t2 || x > t1);
        }

        // AA correction polynomial to be added THIS sample.
        // x is 0-1 samples after the discontinuity.
        inline float BlepBefore(float x) {
            return x * x;
        }

        // AA correction polynomial to be added NEXT sample.
        // x is 0-1 samples after the discontinuity.
        inline float BlepAfter(float x) {
            x = 1.0f - x;
            return -x * x;
        }

        inline float BlampBefore(float x) {
            static constexpr float OneThird = 1.0f / 3.0f;
            return x * x * x * OneThird;
        }

        inline float BlampAfter(float x) {
            static constexpr float NegOneThird = -1.0f / 3.0f;
            x = x - 1.0f;
            return NegOneThird * x * x * x;
        }

        // for detune & unisono, based on enabled oscillators etc, distribute a [-1,1] value among many items.
        // there could be various ways of doing this but to save space just unify.
        // if an odd number of items, then 1 = centered @ 0.0f.
        // then, the remaining even # of items gets + and - respectively, distributed between 0 - 1.
        static void BipolarDistribute(size_t count, const bool* enabled, float* outp)
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
                if (!enabled[iElement]) {
                    // no calc needed; for disabled elements ignore.
                    continue;
                }
                if (iPair >= pairCount) {
                    // past the expected # of pairs; this means it's centered. there should only ever be 1 of these elements (the odd one)
                    outp[iElement] = 0;
                    ++ iEnabled;
                    continue;
                }
                float val = float(iPair + 1) / pairCount; // +1 so 1. we don't use 0; that's reserved for the odd element, and 2. so we hit +1.0f
                if (iEnabled & 1) { // is this the 1st or 2nd in the pair
                    val = -val;
                    ++ iPair;
                }
                ++iEnabled;
                outp[iElement] = val;
            }
        }

        // raw param values are stored in a huge array. the various sub-objects (osc, filters, envelopes)
        // will refer to param values in that array using this. sub classes of this class can do whatever
        // needed alterations mappings etc.
        struct Float01RefParam
        {
        protected:
            real_t& mParamValue;

        public:
            explicit Float01RefParam(real_t& ref) : mParamValue(ref)
            {}
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
            explicit Float01Param(real_t& ref, real_t initialValue) : Float01RefParam(ref, initialValue) {}
            explicit Float01Param(real_t& ref) : Float01RefParam(ref) {}
            real_t Get01Value(real_t modVal = 0.0f) const {
                return mParamValue + modVal;
            }
        };

        struct FloatN11Param : Float01RefParam
        {
            explicit FloatN11Param(real_t& ref, real_t initialValueN11) : Float01RefParam(ref, initialValueN11*.5f+.5f) {
            }
            explicit FloatN11Param(real_t& ref) : Float01RefParam(ref) {
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
            explicit EnvTimeParam(real_t& ref) : Float01RefParam(ref) {}

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
            explicit CurveParam(real_t& ref, real_t initialValueN11) : FloatN11Param(ref, initialValueN11) {}
            explicit CurveParam(real_t& ref) : FloatN11Param(ref) {}

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
            explicit IntParam(real_t& ref, int minValueInclusive, int maxValueInclusive) :
                Float01Param(ref),
                mMinValueInclusive(minValueInclusive),
                mMaxValueInclusive(maxValueInclusive)
            {
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
            explicit EnumParam(real_t& ref, T maxValue) : IntParam(ref, 0, (int)maxValue)
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
            explicit BoolParam(real_t& ref, bool initialValue) : IntParam(ref, 0, 1, initialValue ? 1 : 0)
            {}
            explicit BoolParam(real_t& ref) : IntParam(ref, 0, 1)
            {}

            bool GetBoolValue() const {
                return mParamValue > 0.5f;
            }
            void SetBoolValue(bool b) {
                mParamValue = b ? 1.0f : 0.0f;
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
            explicit VolumeParam(real_t& ref, real_t maxDecibels) :
                Float01Param(ref),
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
            const float mScale;

        public:
            Float01Param mValue;
            Float01Param mKTValue; // how much key tracking to apply. when 0, the frequency doesn't change based on playing note. when 1, it scales completely with the note's frequency.

            explicit FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/, real_t initialValue, real_t initialKT) :
                mValue(valRef, initialValue),
                mKTValue(ktRef, initialKT),
                mCenterFrequency(centerFrequency),
                mCenterMidiNote(FrequencyToMIDINote(centerFrequency)),
                mScale(scale)
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
                param *= mScale;// 10.0f; // (.3 = -2, .8 = 3)
                float fact = math::pow(2, param);
                return Clamp(centerFreq * fact, 0.0f, 22050.0f);
            }

            // param modulation is normal krate param mod
            // noteModulation includes osc.mPitchFine + osc.mPitchSemis + detune;
            float GetMidiNote(float playingMidiNote, float paramModulation) const
            {
                float ktNote = playingMidiNote + 24; // center represents playing note + 2 octaves.

                float centerNote = Lerp(mCenterMidiNote, ktNote, mKTValue.Get01Value());

                float param = mValue.Get01Value() + paramModulation;

                param = (param - 0.5f) * mScale;// 10; // rescale from 0-1 to -5 to +5 (octaves)
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
            OscillatorDetune,
            UnisonoDetune,
            OscillatorSpread,
            UnisonoStereoSpread,
            OscillatorShapeSpread,
            UnisonoShapeSpread,
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
            {"OscDet"}, \
            {"UniDet"}, \
            {"OscSpr"}, \
            {"UniSpr"}, \
            {"OscShp"}, \
            {"UniShp"}, \
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








