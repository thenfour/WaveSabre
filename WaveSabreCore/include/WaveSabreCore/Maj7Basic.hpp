#pragma once

#include <WaveSabreCore/Helpers.h>
#include <memory>

// correction for windows.h macros.
// i cannot use NOMINMAX, because VST and Gdiplus depend on these macros. 
#undef min
#undef max

#include <algorithm>

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

        static constexpr real_t gFilterCenterFrequency = 1000.0f;
        static constexpr real_t gFilterFrequencyScale = 10.0f;
        static constexpr float MIN_DECIBEL_GAIN = -60.0f;

        static constexpr real_t gLFOLPCenterFrequency = 20.0f;
        static constexpr real_t gLFOLPFrequencyScale = 7.0f;

        static constexpr size_t gModulationCount = 16;

        // too high (1023 for example) results in ugly audible ramping between modulations
        // 255 feels actually quite usable
        // 127 as well. - maybe this could be a "low cpu" setting.
        // 31 feels like a good quality compromise.
        // 7 is sharp quality.
        // 3 is HD.
        enum class QualitySetting : uint8_t
        {
            Potato,
            Carrot,
            Cauliflower,
            Celery,
            Artichoke,
            Count,
        };

        #define QUALITY_SETTING_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::QualitySetting::Count]{ \
                "Potato", \
                "Carrot", \
                "Cauliflower", \
                "Celery", \
                "Artichoke",\
        };

        extern uint16_t GetAudioOscillatorRecalcSampleMask();
        extern uint16_t GetModulationRecalcSampleMask();
        
        extern QualitySetting GetQualitySetting();
        extern void SetQualitySetting(QualitySetting);

        static const float gMinGainLinear = 0.001f;// DecibelsToLinear(MIN_DECIBEL_GAIN); // avoid dynamic initializer

        namespace math
        {
            static constexpr real_t gPI = 3.14159265358979323846264338327950288f;
            static constexpr real_t gPITimes2 = gPI * 2;

            inline real_t floor(real_t x) {
                return (real_t)::floor((double)x);
            }
            inline double floord(double x) {
                return ::floor(x);
            }
            inline float rand01() {
                return float(::rand()) / RAND_MAX;
            }
            inline float randN11() {
                return rand01() * 2 - 1;
            }
            inline constexpr real_t max(real_t x, real_t y) {
                return x > y ? x : y;
            }
            inline constexpr real_t min(real_t x, real_t y) {
                return x < y ? x : y;
            }
            inline constexpr float abs(float x)
            {
                return x < 0 ? -x : x;
            }
            inline constexpr real_t clamp(real_t x, real_t low, real_t hi)
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

            bool FloatEquals(real_t f1, real_t f2, real_t eps = FloatEpsilon);

            bool FloatLessThanOrEquals(real_t lhs, real_t rhs, real_t eps = FloatEpsilon);

            float lerp(float a, float b, float t);

            inline float lerp_rev(float v_min, float v_max, float v_val) // inverse of lerp; returns 0-1 where t lies between a and b.
            {
                return (v_val - v_min) / (v_max - v_min);
            }
            inline float map(float inp, float inp_min, float inp_max, float out_min, float out_max)
            {
                float t = lerp_rev(inp_min, inp_max, inp);
                return lerp(out_min, out_max, t);
            }

            real_t CalculateInc01PerSampleForMS(real_t ms);


            inline bool IsSilentGain(float gain)
            {
                return gain <= gMinGainLinear;
            }

            inline float MillisecondsToSamples(float ms)
            {
                static constexpr float oneOver1000 = 1.0f / 1000.0f; // obsessive optimization?
                return (ms * Helpers::CurrentSampleRateF) * oneOver1000;
            }


            inline real_t pow(real_t x, real_t y) {
                // fasterpow is just too inaccurate.
                //return fastmath::fastpow(x, y); // this is also not always accurate, just due to ranges. 
                return ::powf(x, y);
            }
            inline real_t pow2(real_t y) {
                return pow(2, y);
            }
            inline float log2(float x) {
                static constexpr float log2_e = 1.44269504089f; // 1/log(2)
                return (float)::logl((double)x) * log2_e;
            }
            inline float log10(float x) {
                static constexpr float log10_e = 0.4342944819f;// 1.0f / LOG(10.0f);
                return (float)::logl((double)x) * log10_e;
            }
            inline real_t tan(real_t x) {
                return (float)::tan((double)x);// fastmath::fastertanfull(x); // less fast to call lib function, but smaller code.
            }

            inline float fract(float x) {
                return x - math::floor(x);
            }

            inline double fract(double x) {
                return x - math::floord(x);
            }

            // where t1, t2, and x are periodic values [0,1).
            // and t1 is "before" t2,
            // return true if x falls between t1 and t2.
            // so if t2 < t1, it means a cycle wrap.
            bool DoesEncounter(double t1, double t2, float x);


            // lookup table for 0-1 input values, linear interpolation upon lookup.
            // non-periodic.
            struct LUT01
            {
                const size_t mNSamples;
                float* mpTable;

                LUT01(size_t nSamples, float (*fn)(float));
                //virtual ~LUT01();
                virtual float Invoke(float x) const;
            };

            struct SinCosLUT : public LUT01
            {
                SinCosLUT(size_t nSamples, float (*fn)(float));

                virtual float Invoke(float x) const override;
            };

            // tanh approaches -1 before -PI, and +1 after +PI. so squish the range and do a 0,1 LUT mapping from -PI,PI
            struct TanHLUT : public LUT01
            {
                TanHLUT(size_t nSamples);
                virtual float Invoke(float x) const override;
            };

            struct LUT2D {
                size_t mNSamplesX;
                size_t mNSamplesY;
                float* mpTable;

                LUT2D(size_t nSamplesX, size_t nSamplesY, float (*fn)(float, float));

                //virtual ~LUT2D();

                virtual float Invoke(float x, float y) const;
            };

            struct CurveLUT : public LUT2D
            {
                // valid for 0<k<1 and 0<x<1
                static real_t modCurve_x01_k01_RT(real_t x, real_t k);

                // extends range to support -1<x<0 and -1<k<0
                // outputs -1 to 1
                static real_t modCurve_xN11_kN11_RT(real_t x, real_t k);

                // incoming values should support -1,1 and output -1,1
                CurveLUT(size_t nSamples);

                // user passes in n11 values; need to map N11 to 01
                        virtual float Invoke(float xN11, float yN11) const override;
            };

            extern SinCosLUT gSinLUT;
            extern SinCosLUT gCosLUT;
            extern TanHLUT gTanhLUT;
            extern LUT01 gSqrt01LUT;// sqrt 01

            extern CurveLUT gCurveLUT;
            // 2D LUTs: pow() 0-1, and curve


            // pow(2,n) is a quite hot path, used by MidiNoteToFrequency(), as well as all other frequency calculations.
            // the range of `n` is [-15,+15] but depends on frequency param scale. so let's extend a bit and make a huge lut.
            struct Pow2_N16_16_LUT : public LUT01
            {
                Pow2_N16_16_LUT(size_t nSamples);
                virtual float Invoke(float x) const override;
            };
            extern Pow2_N16_16_LUT gPow2_N16_16_LUT;

            inline float pow2_N16_16(float n)
            {
                return gPow2_N16_16_LUT.Invoke(n);
            }

            inline real_t sin(real_t x) {
                return gSinLUT.Invoke(x);
                //return (real_t)Helpers::FastSin((double)x);
            }
            inline real_t cos(real_t x) {
                return gCosLUT.Invoke(x);// (real_t)Helpers::FastCos((double)x);
            }

            inline real_t sqrt(real_t x) {
                return (float)::sqrt((double)x);
            }

            // optimized LUT works for 0<=x<=1
            inline real_t sqrt01(real_t x) {
                return gSqrt01LUT.Invoke(x);
            }

            inline real_t tanh(real_t x) {
                return gTanhLUT.Invoke(x);
                //return (float)::tanh((double)x);
                //return fastmath::fastertanh(x); // tanh is used for saturation and the fast version adds weird high frequency content. try it on a LP filter and see what i mean.
            }

            inline real_t modCurve_xN11_kN11(real_t x, real_t k)
            {
                return gCurveLUT.Invoke(x, k);
            }

            /**
             * Converts a linear value to decibels.  Returns <= aMinDecibels if the linear
             * value is 0.
             */
            float LinearToDecibels(float aLinearValue, float aMinDecibels = MIN_DECIBEL_GAIN);

            /**
             * Converts a decibel value to a linear value.
             */
            float DecibelsToLinear(float aDecibels, float aNegInfDecibels = MIN_DECIBEL_GAIN);

            // don't use a LUT because we want to support pitch bend and glides and stuff. using a LUT + interpolation would be
            // asinine.
            float MIDINoteToFreq(float x);
            float SemisToFrequencyMul(float x);

            float FrequencyToMIDINote(float hz);
            std::pair<float, float> PanToLRVolumeParams(float panN11);

            std::pair<float, float> PanToFactor(float panN11);
        } // namespace math


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
        void BipolarDistribute(size_t count, const bool* enabled, float* outp);

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
            //explicit Float01Param(real_t& ref, real_t initialValue);
            explicit Float01Param(real_t& ref);
            real_t Get01Value(real_t modVal = 0.0f) const;
        };

        struct FloatN11Param : Float01RefParam
        {
            float mCachedVal;
            //explicit FloatN11Param(real_t& ref, real_t initialValueN11);
            explicit FloatN11Param(real_t& ref);

            real_t GetN11Value(real_t modVal = 0.0f) const;
            void SetN11Value(real_t v);
            inline float CacheValue() {
                return mCachedVal = GetN11Value();
            }
        };

        // todo: time should not always be the same; allow configuring the range.
        // min value is always 0 milliseconds.
        struct EnvTimeParam : Float01RefParam
        {
            explicit EnvTimeParam(real_t& ref, real_t initialParamValue);
            explicit EnvTimeParam(real_t& ref);

            static constexpr real_t gCenterValue = 375; // the MS at 0.5 param value.
            static constexpr int gRangeLog2 = 10;      // must be even for below calculations to work
            static constexpr real_t gMinRawVal =
                (1.0f / (1 << (gRangeLog2 / 2))) *
                gCenterValue; // minimum value as described by log calc (before subtracting min).
            static constexpr real_t gMaxRawVal = (1 << (gRangeLog2 / 2)) * gCenterValue;
            static constexpr real_t gMinRealVal = 0;
            static constexpr real_t gMaxRealVal = gMaxRawVal - gMinRawVal;

            real_t GetMilliseconds(real_t paramModulation = 0.0f) const;
        };

        // paramvalue N11
        struct CurveParam : FloatN11Param
        {
            //explicit CurveParam(real_t& ref, real_t initialValueN11) : FloatN11Param(ref, initialValueN11) {}
            explicit CurveParam(real_t& ref) : FloatN11Param(ref) {}

            real_t ApplyToValue(real_t x, real_t modVal = 0.0f) const {
                float k = GetN11Value() + modVal;
                if (k < 0.0001 && k > -0.0001) return x; // speed optimization; most curves being processed are flat so skip the lookup entirely.
                return math::modCurve_xN11_kN11(x, k);
            }
        };

        // stores an integral parameter as a 0-1 float param
        struct IntParam : Float01Param
        {
        protected:
            const int mMinValueInclusive;
            const int mMaxValueInclusive;
            const float mHalfMinusMinVal; // precalc to avoid an add
        public:
            int mCachedVal;
            //explicit IntParam(real_t& ref, int minValueInclusive, int maxValueInclusive, int initialValue);
            explicit IntParam(real_t& ref, int minValueInclusive, int maxValueInclusive);
            int GetDiscreteValueCount() const;
            // we want to split the float 0-1 range into equal portions. so if you have 3 values (0 - 2 inclusive),
            //     0      1        2
            // |------|-------|-------|
            // 0     .33     .66     1.00
            int GetIntValue() const;
            void SetIntValue(int val);
            inline int CacheValue() {
                return mCachedVal = GetIntValue();
            }
        };

        // so i had this idea:
        //   "in order to ease backwards compatibility, we assume each enum < 2000 elements.
        //      that way when you add a new enum value at the end, it doesn't break all values." 
        // that's fine but has some side-effects i don't want:
        // - it means the param range in the DAW is impossible to use. not that these enum values are automated much but worth mentioning.
        // - it feels deceptive that the range is advertised as huge, but the real range is small. for serialization of enums for example this just feels like spaghetti where assumptions are incompatible.
        // that being said it's still practical, as these are not the kinds of params that get automated via an envelope.
        template <typename T>
        struct EnumParam : IntParam
        {
            static constexpr size_t MaxItems = 2023;
            T mCachedVal;
            //explicit EnumParam(real_t& ref, T maxValue, T initialValue) : IntParam(ref, 0, MaxItems, (int)initialValue)
            //{}
            explicit EnumParam(real_t& ref, T maxValue) : IntParam(ref, 0, MaxItems)
            {
                CacheValue();
            }
            T GetEnumValue() const {
                return (T)GetIntValue();
            }
            void SetEnumValue(T v) {
                SetIntValue((int)v);
                mCachedVal = v;
            }
            inline T CacheValue() {
                return mCachedVal = GetEnumValue();
            }
        };

        struct BoolParam
        {
        private:
            float& mVal;
        public:
            bool mCachedVal = false;
            inline explicit BoolParam(real_t& ref) : mVal(ref) {
                CacheValue();
            }
            inline bool GetBoolValue() const { return mVal > 0.5f; } // this generates slightly smaller code than == 0, or even >0
            inline void SetBoolValue(bool b) { mVal = float(b ? 1 : 0); }
            inline void SetRawParamValue(float f) { mVal = f; }
            inline bool CacheValue() {
                return mCachedVal = GetBoolValue();
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
            float mCachedVal;
            //explicit ScaledRealParam(real_t& ref, real_t minValueInclusive, real_t maxValueInclusive, real_t initialRangedValue);
            explicit ScaledRealParam(real_t& ref, real_t minValueInclusive, real_t maxValueInclusive);
            real_t GetRange() const;
            real_t GetRangedValue() const;
            void SetRangedValue(real_t val);
            inline float CacheValue() {
                return mCachedVal = GetRangedValue();
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

            float ParamToLinear(float x) const;
            float LinearToParam(float x) const;
            float ParamToDecibels(float x) const;
            float DecibelsToParam(float db) const;

        public:
            //explicit VolumeParam(real_t& ref, real_t maxDecibels, real_t initialParamValue01);
            explicit VolumeParam(real_t& ref, real_t maxDecibels);
            float GetLinearGain(float modVal = 0.0f) const;
            float GetDecibels() const;
            float IsSilent() const;
            void SetLinearValue(float f);
            void SetDecibels(float db);
        };

        struct WSQParam : Float01Param
        {
            explicit WSQParam(real_t& ref) : Float01Param(ref) {}
            float GetQValue() const {
                return Helpers::ParamToQ(this->mParamValue);
            }
            void SetQValue(float f) {
                mParamValue = Helpers::QToParam(f);
            }

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

            //explicit FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/, real_t initialValue, real_t initialKT);
            explicit FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/);
            // noteHz is the playing note, to support key-tracking.
            float GetFrequency(float noteHz, float paramModulation) const;
            void SetFrequencyAssumingNoKeytracking(float hz);
            // param modulation is normal krate param mod
            // noteModulation includes osc.mPitchFine + osc.mPitchSemis + detune;
            float GetMidiNote(float playingMidiNote, float paramModulation) const;
        };

        struct Float01ParamArray
        {
            float* mpArray;
            size_t mCount;
            Float01ParamArray(float* parr, size_t count) : mpArray(parr), mCount(count) {}
            float Get01Value(size_t i) const {
                return math::clamp(mpArray[i], 0, 1);
            }
            float Get01Value(size_t i, float mod) const {
                return math::clamp(mpArray[i] + mod, 0, 1);
            }
        };

        //template<typename T, typename = std::enable_if_t<std::is_integral_v<T>&& std::is_unsigned_v<T>>>
        //struct Bitfield
        //{
        //    T mVal = 0;
        //    Bitfield() {}
        //    explicit Bitfield(T val) : mVal(val) {}

        //    template<size_t bit>
        //    bool ReadBit() {
        //        return !!(mVal & (1 << bit));
        //    }
        //    bool ReadBit(size_t bit) {
        //        return !!(mVal & (1 << bit));
        //    }
        //    template<size_t firstBit, size_t numBits>
        //    T ReadBits() {
        //        return (mVal >> firstBit) & ((1 << numBits) - 1);
        //    }
        //    template<size_t firstBit, size_t numBits, typename Tenum>
        //    Tenum ReadEnum() {
        //        static_assert(std::is_enum_v<Tenum>, "Tenum must be an enumeration type");
        //        return (Tenum)ExtractBits(firstBit, numBits);
        //    }

        //    template<size_t bit>
        //    void WriteBit(bool b)
        //    {
        //        T mask = 1 << bit;
        //        if (b) { mVal |= mask; }
        //        else { mVal &= ~mask; }
        //    }

        //    void WriteBit(size_t bit, bool b)
        //    {
        //        T mask = 1 << bit;
        //        if (b) { mVal |= mask; }
        //        else { mVal &= ~mask; }
        //    }

        //    template<size_t firstBit, size_t numBits>
        //    T WriteValue(T valueToWrite)
        //    {
        //        static_assert(firstBit + numBits <= sizeof(T) * 8, "Bits out of range");
        //        static constexpr T mask = (1 << numBits) - 1;
        //        T encodedVal = valueToWrite & mask;
        //        encodedVal <<= firstBit;
        //        //T maskedVal = mVal & ~(mask << shift); assume the space is already vacant.
        //        T result = (mVal | encodedVal);
        //        mVal = result;
        //        return result;
        //    }

        //    template<size_t firstBit, size_t numBits, typename Tenum>
        //    void WriteEnum(Tenum valueToWrite)
        //    {
        //        static_assert(std::is_enum_v<Tenum>, "Tenum must be an enumeration type");
        //        WriteValue<firstBit, numBits>(static_cast<std::underlying_type_t<Tenum>>(valueToWrite));
        //    }
        //};

        //using ByteBitfield = Bitfield<uint8_t>;
        //using WordBitfield = Bitfield<uint16_t>;

        struct Serializer
        {
            uint8_t* mBuffer = nullptr;
            size_t mAllocatedSize = 0;
            size_t mSize = 0;

            ~Serializer();

            std::pair<uint8_t*, size_t> DetachBuffer();
            uint8_t* GrowBy(size_t n);
            void WriteUByte(uint8_t b);
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
            void WriteFloat(float f);
            void WriteUInt32(uint32_t f);
            void WriteBuffer(const uint8_t* buf, size_t n);
        };

        // attaches to some buffer
        struct Deserializer
        {
            const uint8_t* mpData;
            const uint8_t* mpCursor;
            const uint8_t* mpEnd;
            size_t mSize;
            explicit Deserializer(const uint8_t* p, size_t n);
            //int8_t ReadSByte() {
            //    int8_t ret = *((int8_t*)mpCursor);
            //    mpCursor += sizeof(ret);
            //    return ret;
            //}
            uint8_t ReadUByte();

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

            uint32_t ReadUInt32();

            float ReadFloat();
            // returns a new cursor in the out buffer 
            void ReadBuffer(void* out, size_t numbytes);

            //ByteBitfield ReadByteBitfield() {
            //    return ByteBitfield{ ReadUByte() };
            //}
            //WordBitfield ReadWordBitfield() {
            //    return WordBitfield{ ReadUWord() };
            //}
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
            //OscillatorShapeSpread,
            //UnisonoShapeSpread,
            FMBrightness,

            AuxRouting,
            AuxWidth, // N11 to invert

            PortamentoTime,
            PortamentoCurve,
            PitchBendRange,

            Macro1,
            Macro2,
            Macro3,
            Macro4,
            Macro5,
            Macro6,
            Macro7,

            FMAmt2to1,
            FMAmt3to1,
            FMAmt4to1,
            FMAmt1to2,
            FMAmt3to2,
            FMAmt4to2,
            FMAmt1to3,
            FMAmt2to3,
            FMAmt4to3,
            FMAmt1to4,
            FMAmt2to4,
            FMAmt3to4,

            Osc1Enabled, // KEEP IN SYNC WITH OscParamIndexOffsets
            Osc1Volume,
            Osc1KeyrangeMin,
            Osc1KeyrangeMax,
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
            Osc1AuxMix,

            Osc1AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
            Osc1AmpEnvAttackTime,
            Osc1AmpEnvAttackCurve,
            Osc1AmpEnvHoldTime,
            Osc1AmpEnvDecayTime,
            Osc1AmpEnvDecayCurve,
            Osc1AmpEnvSustainLevel,
            Osc1AmpEnvReleaseTime,
            Osc1AmpEnvReleaseCurve,
            Osc1AmpEnvLegatoRestart,

            Osc2Enabled, // KEEP IN SYNC WITH OscParamIndexOffsets
            Osc2Volume,
            Osc2KeyrangeMin,
            Osc2KeyrangeMax,
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
            Osc2AuxMix,

            Osc2AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
            Osc2AmpEnvAttackTime,
            Osc2AmpEnvAttackCurve,
            Osc2AmpEnvHoldTime,
            Osc2AmpEnvDecayTime,
            Osc2AmpEnvDecayCurve,
            Osc2AmpEnvSustainLevel,
            Osc2AmpEnvReleaseTime,
            Osc2AmpEnvReleaseCurve,
            Osc2AmpEnvLegatoRestart,

            Osc3Enabled, // KEEP IN SYNC WITH OscParamIndexOffsets
            Osc3Volume,
            Osc3KeyrangeMin,
            Osc3KeyrangeMax,
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
            Osc3AuxMix,

            Osc3AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
            Osc3AmpEnvAttackTime,
            Osc3AmpEnvAttackCurve,
            Osc3AmpEnvHoldTime,
            Osc3AmpEnvDecayTime,
            Osc3AmpEnvDecayCurve,
            Osc3AmpEnvSustainLevel,
            Osc3AmpEnvReleaseTime,
            Osc3AmpEnvReleaseCurve,
            Osc3AmpEnvLegatoRestart,

                Osc4Enabled, // KEEP IN SYNC WITH OscParamIndexOffsets
                Osc4Volume,
                Osc4KeyrangeMin,
                Osc4KeyrangeMax,
                Osc4Waveform,
                Osc4Waveshape,
                Osc4PhaseRestart,
                Osc4PhaseOffset,
                Osc4SyncEnable,
                Osc4SyncFrequency,
                Osc4SyncFrequencyKT,
                Osc4FrequencyParam,
                Osc4FrequencyParamKT,
                Osc4PitchSemis,
                Osc4PitchFine,
                Osc4FreqMul,
                Osc4FMFeedback,
                Osc4AuxMix,
                   
                Osc4AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
                Osc4AmpEnvAttackTime,
                Osc4AmpEnvAttackCurve,
                Osc4AmpEnvHoldTime,
                Osc4AmpEnvDecayTime,
                Osc4AmpEnvDecayCurve,
                Osc4AmpEnvSustainLevel,
                Osc4AmpEnvReleaseTime,
                Osc4AmpEnvReleaseCurve,
                Osc4AmpEnvLegatoRestart,

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
            LFO1Sharpness,

            LFO2Waveform, // KEEP IN SYNC WITH LFOParamIndexOffsets
            LFO2Waveshape,
            LFO2Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
            LFO2PhaseOffset,
            LFO2FrequencyParam,
            LFO2Sharpness,

                Aux1Enabled,
                Aux1Link,
                Aux1Type,
                Aux1Param1, // filter type
                Aux1Param2, // filter Q
                Aux1Param3, // filter saturation
                Aux1Param4, // filter freq
                Aux1Param5, // filter KT

                Aux2Enabled,
                Aux2Link,
                Aux2Type,
                Aux2Param1, // filter type
                Aux2Param2, // filter Q
                Aux2Param3, // filter saturation
                Aux2Param4, // filter freq
                Aux2Param5, // filter KT

                Aux3Enabled,
                Aux3Link,
                Aux3Type,
                Aux3Param1, // filter type
                Aux3Param2, // filter Q
                Aux3Param3, // filter saturation
                Aux3Param4, // filter freq
                Aux3Param5, // filter KT

                Aux4Enabled,
                Aux4Link,
                Aux4Type,
                Aux4Param1, // filter type
                Aux4Param2, // filter Q
                Aux4Param3, // filter saturation
                Aux4Param4, // filter freq
                Aux4Param5, // filter KT

            Mod1Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod1Source,
            Mod1Destination,
            Mod1Curve,
            Mod1Scale,
            Mod1AuxEnabled,
            Mod1AuxSource,
            Mod1AuxAttenuation,
            Mod1AuxCurve,
            Mod1Invert,
            Mod1AuxInvert,

            Mod2Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod2Source,
            Mod2Destination,
            Mod2Curve,
            Mod2Scale,
            Mod2AuxEnabled,
            Mod2AuxSource,
            Mod2AuxAttenuation,
            Mod2AuxCurve,
                Mod2Invert,
                Mod2AuxInvert,

            Mod3Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod3Source,
            Mod3Destination,
            Mod3Curve,
            Mod3Scale,
            Mod3AuxEnabled,
            Mod3AuxSource,
            Mod3AuxAttenuation,
            Mod3AuxCurve,
                Mod3Invert,
                Mod3AuxInvert,

            Mod4Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod4Source,
            Mod4Destination,
            Mod4Curve,
            Mod4Scale,
            Mod4AuxEnabled,
            Mod4AuxSource,
            Mod4AuxAttenuation,
            Mod4AuxCurve,
                Mod4Invert,
                Mod4AuxInvert,

            Mod5Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod5Source,
            Mod5Destination,
            Mod5Curve,
            Mod5Scale,
            Mod5AuxEnabled,
            Mod5AuxSource,
            Mod5AuxAttenuation,
            Mod5AuxCurve,
                Mod5Invert,
                Mod5AuxInvert,

            Mod6Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod6Source,
            Mod6Destination,
            Mod6Curve,
            Mod6Scale,
            Mod6AuxEnabled,
            Mod6AuxSource,
            Mod6AuxAttenuation,
            Mod6AuxCurve,
                Mod6Invert,
                Mod6AuxInvert,

            Mod7Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod7Source,
            Mod7Destination,
            Mod7Curve,
            Mod7Scale,
            Mod7AuxEnabled,
            Mod7AuxSource,
            Mod7AuxAttenuation,
            Mod7AuxCurve,
                Mod7Invert,
                Mod7AuxInvert,

            Mod8Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod8Source,
            Mod8Destination,
            Mod8Curve,
            Mod8Scale,
            Mod8AuxEnabled,
            Mod8AuxSource,
            Mod8AuxAttenuation,
            Mod8AuxCurve,
                Mod8Invert,
                Mod8AuxInvert,

                Mod9Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod9Source,
                Mod9Destination,
                Mod9Curve,
                Mod9Scale,
                Mod9AuxEnabled,
                Mod9AuxSource,
                Mod9AuxAttenuation,
                Mod9AuxCurve,
                Mod9Invert,
                Mod9AuxInvert,

                Mod10Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod10Source,
                Mod10Destination,
                Mod10Curve,
                Mod10Scale,
                Mod10AuxEnabled,
                Mod10AuxSource,
                Mod10AuxAttenuation,
                Mod10AuxCurve,
                Mod10Invert,
                Mod10AuxInvert,

                Mod11Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod11Source,
                Mod11Destination,
                Mod11Curve,
                Mod11Scale,
                Mod11AuxEnabled,
                Mod11AuxSource,
                Mod11AuxAttenuation,
                Mod11AuxCurve,
                Mod11Invert,
                Mod11AuxInvert,

                Mod12Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod12Source,
                Mod12Destination,
                Mod12Curve,
                Mod12Scale,
                Mod12AuxEnabled,
                Mod12AuxSource,
                Mod12AuxAttenuation,
                Mod12AuxCurve,
                Mod12Invert,
                Mod12AuxInvert,

                Mod13Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod13Source,
                Mod13Destination,
                Mod13Curve,
                Mod13Scale,
                Mod13AuxEnabled,
                Mod13AuxSource,
                Mod13AuxAttenuation,
                Mod13AuxCurve,
                Mod13Invert,
                Mod13AuxInvert,

                Mod14Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod14Source,
                Mod14Destination,
                Mod14Curve,
                Mod14Scale,
                Mod14AuxEnabled,
                Mod14AuxSource,
                Mod14AuxAttenuation,
                Mod14AuxCurve,
                Mod14Invert,
                Mod14AuxInvert,

                Mod15Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod15Source,
                Mod15Destination,
                Mod15Curve,
                Mod15Scale,
                Mod15AuxEnabled,
                Mod15AuxSource,
                Mod15AuxAttenuation,
                Mod15AuxCurve,
                Mod15Invert,
                Mod15AuxInvert,

                Mod16Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod16Source,
                Mod16Destination,
                Mod16Curve,
                Mod16Scale,
                Mod16AuxEnabled,
                Mod16AuxSource,
                Mod16AuxAttenuation,
                Mod16AuxCurve,
                Mod16Invert,
                Mod16AuxInvert,

                Sampler1Enabled, // KEEP IN SYNC WITH SamplerParamIndexOffsets
                Sampler1Volume,
                Sampler1KeyrangeMin,
                Sampler1KeyrangeMax,
                Sampler1BaseNote,
                Sampler1LegatoTrig,
                Sampler1Reverse,
                Sampler1Source, // gmdls or own sample
                Sampler1GmDlsIndex,
                Sampler1SampleStart,
                Sampler1LoopMode,
                Sampler1LoopSource,
                Sampler1LoopStart,
                Sampler1LoopLength,
                Sampler1TuneSemis,
                Sampler1TuneFine,
                Sampler1FreqParam,
                Sampler1FreqKT,
                Sampler1InterpolationType,
                Sampler1ReleaseExitsLoop,
                Sampler1AuxMix,

                Sampler1AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
                Sampler1AmpEnvAttackTime,
                Sampler1AmpEnvAttackCurve,
                Sampler1AmpEnvHoldTime,
                Sampler1AmpEnvDecayTime,
                Sampler1AmpEnvDecayCurve,
                Sampler1AmpEnvSustainLevel,
                Sampler1AmpEnvReleaseTime,
                Sampler1AmpEnvReleaseCurve,
                Sampler1AmpEnvLegatoRestart,

                    Sampler2Enabled, // KEEP IN SYNC WITH SamplerParamIndexOffsets
                    Sampler2Volume,
                Sampler2KeyrangeMin,
                Sampler2KeyrangeMax,
                Sampler2BaseNote,
                Sampler2LegatoTrig,
                    Sampler2Reverse,
                    Sampler2Source, // gmdls or own sample
                    Sampler2GmDlsIndex,
                    Sampler2SampleStart,
                    Sampler2LoopMode,
                    Sampler2LoopSource,
                    Sampler2LoopStart,
                    Sampler2LoopLength,
                    Sampler2TuneSemis,
                    Sampler2TuneFine,
                    Sampler2FreqParam,
                    Sampler2FreqKT,
                    Sampler2InterpolationType,
                Sampler2ReleaseExitsLoop,
                Sampler2AuxMix,
                           
                    Sampler2AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
                    Sampler2AmpEnvAttackTime,
                    Sampler2AmpEnvAttackCurve,
                    Sampler2AmpEnvHoldTime,
                    Sampler2AmpEnvDecayTime,
                    Sampler2AmpEnvDecayCurve,
                    Sampler2AmpEnvSustainLevel,
                    Sampler2AmpEnvReleaseTime,
                    Sampler2AmpEnvReleaseCurve,
                    Sampler2AmpEnvLegatoRestart,

                Sampler3Enabled, // KEEP IN SYNC WITH SamplerParamIndexOffsets
                Sampler3Volume,
                    Sampler3KeyrangeMin,
                    Sampler3KeyrangeMax,
                    Sampler3BaseNote,
                    Sampler3LegatoTrig,
                Sampler3Reverse,
                    Sampler3Source, // gmdls or own sample
                    Sampler3GmDlsIndex,
                    Sampler3SampleStart,
                Sampler3LoopMode,
                Sampler3LoopSource,
                Sampler3LoopStart,
                Sampler3LoopLength,
                Sampler3TuneSemis,
                Sampler3TuneFine,
                Sampler3FreqParam,
                Sampler3FreqKT,
                Sampler3InterpolationType,
                Sampler3ReleaseExitsLoop,
                Sampler3AuxMix,
                       
                Sampler3AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
                Sampler3AmpEnvAttackTime,
                Sampler3AmpEnvAttackCurve,
                Sampler3AmpEnvHoldTime,
                Sampler3AmpEnvDecayTime,
                Sampler3AmpEnvDecayCurve,
                Sampler3AmpEnvSustainLevel,
                Sampler3AmpEnvReleaseTime,
                Sampler3AmpEnvReleaseCurve,
                Sampler3AmpEnvLegatoRestart,

                Sampler4Enabled, // KEEP IN SYNC WITH SamplerParamIndexOffsets
                Sampler4Volume,
                Sampler4KeyrangeMin,
                Sampler4KeyrangeMax,
                Sampler4BaseNote,
                Sampler4LegatoTrig,
                Sampler4Reverse,
                    Sampler4Source, // gmdls or own sample
                    Sampler4GmDlsIndex,
                    Sampler4SampleStart,
                Sampler4LoopMode,
                Sampler4LoopSource,
                Sampler4LoopStart,
                Sampler4LoopLength,
                Sampler4TuneSemis,
                Sampler4TuneFine,
                Sampler4FreqParam,
                Sampler4FreqKT,
                Sampler4InterpolationType,
                    Sampler4ReleaseExitsLoop,
                    Sampler4AuxMix,
                       
                Sampler4AmpEnvDelayTime, // KEEP IN SYNC WITH EnvParamIndexOffsets
                Sampler4AmpEnvAttackTime,
                Sampler4AmpEnvAttackCurve,
                Sampler4AmpEnvHoldTime,
                Sampler4AmpEnvDecayTime,
                Sampler4AmpEnvDecayCurve,
                Sampler4AmpEnvSustainLevel,
                Sampler4AmpEnvReleaseTime,
                Sampler4AmpEnvReleaseCurve,
                Sampler4AmpEnvLegatoRestart,

                NumParams,
                Invalid,
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
            {"FMBrigh"}, \
            {"XRout"}, \
            {"XWidth"}, \
		    {"PortTm"}, \
		    {"PortCv"}, \
		    {"PBRng"}, \
		    {"Macro1"}, \
		    {"Macro2"}, \
		    {"Macro3"}, \
		    {"Macro4"}, \
		    {"Macro5"}, \
		    {"Macro6"}, \
		    {"Macro7"}, \
		    {"FM2to1"}, \
		    {"FM3to1"}, \
		    {"FM4to1"}, \
		    {"FM1to2"}, \
		    {"FM3to2"}, \
		    {"FM4to2"}, \
		    {"FM1to3"}, \
		    {"FM2to3"}, \
		    {"FM4to3"}, \
		    {"FM1to4"}, \
		    {"FM2to4"}, \
		    {"FM3to4"}, \
            {"O1En"}, \
		    {"O1Vol"}, \
		    {"O1KRmin"}, \
		    {"O1KRmax"}, \
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
		    {"O1Xmix"}, \
		    {"AE1dlt"}, \
		    {"AE1att"}, \
		    {"AE1atc"}, \
		    {"AE1ht"}, \
		    {"AE1dt"}, \
		    {"AE1dc"}, \
		    {"AE1sl"}, \
		    {"AE1rt"}, \
		    {"AE1tc"}, \
		    {"AE1rst"}, \
		    {"O2En"}, \
		    {"O2Vol"}, \
		    {"O2KRmin"}, \
		    {"O2KRmax"}, \
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
		    {"O2Xmix"}, \
		    {"AE2dlt"}, \
		    {"AE2att"}, \
		    {"AE2atc"}, \
		    {"AE2ht"}, \
		    {"AE2dt"}, \
		    {"AE2dc"}, \
		    {"AE2sl"}, \
		    {"AE2rt"}, \
		    {"AE2tc"}, \
		    {"AE2rst"}, \
		    {"O3En"}, \
		    {"O3Vol"}, \
		    {"O3KRmin"}, \
		    {"O3KRmax"}, \
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
		    {"O3Xmix"}, \
		    {"AE3dlt"}, \
		    {"AE3att"}, \
		    {"AE3atc"}, \
		    {"AE3ht"}, \
		    {"AE3dt"}, \
		    {"AE3dc"}, \
		    {"AE3sl"}, \
		    {"AE3rt"}, \
		    {"AE3tc"}, \
		    {"AE3rst"}, \
            {"O4En"}, \
            {"O4Vol"}, \
            {"O4KRmin"}, \
            {"O4KRmax"}, \
            {"O4Wave"}, \
            {"O4Shp"}, \
            {"O4PRst"}, \
            {"O4Poff"}, \
            {"O4Scen"}, \
            {"O4ScFq"}, \
            {"O4ScKt"}, \
            {"O4Fq"}, \
            {"O4FqKt"}, \
            {"O4Semi"}, \
            {"O4Fine"}, \
            {"O4Mul"}, \
            {"O4FMFb"}, \
            {"O4Xmix"}, \
            {"AE4dlt"}, \
            {"AE4att"}, \
            {"AE4atc"}, \
            {"AE4ht"}, \
            {"AE4dt"}, \
            {"AE4dc"}, \
            {"AE4sl"}, \
            {"AE4rt"}, \
            {"AE4tc"}, \
            {"AE4rst"}, \
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
		    {"LFO1lp"}, \
		    {"LFO2wav"}, \
		    {"LFO2shp"}, \
		    {"LFO2rst"}, \
		    {"LFO2ph"}, \
		    {"LFO2fr"}, \
		    {"LFO2lp"}, \
            {"X1En"}, \
            {"X1Link"}, \
            {"X1Type"}, \
            {"X1P1"}, \
            {"X1P2"}, \
            {"X1P3"}, \
            {"X1P4"}, \
            {"X1P5"}, \
            {"X2En"}, \
            {"X2Link"}, \
            {"X2Type"}, \
            {"X2P1"}, \
            {"X2P2"}, \
            {"X2P3"}, \
            {"X2P4"}, \
            {"X2P5"}, \
            {"X3En"}, \
            {"X3Link"}, \
            {"X3Type"}, \
            {"X31"}, \
            {"X32"}, \
            {"X33"}, \
            {"X34"}, \
            {"X35"}, \
            {"X4En"}, \
            {"X4Link"}, \
            {"X4Type"}, \
            {"X41"}, \
            {"X42"}, \
            {"X43"}, \
            {"X44"}, \
            {"X45"}, \
		    {"M1en"}, \
		    {"M1src"}, \
		    {"M1dest"}, \
		    {"M1curv"}, \
		    {"M1scale"}, \
            {"M1Aen"}, \
            {"M1Asrc"}, \
            {"M1Aatt"}, \
            {"M1Acrv"}, \
            {"M1inv"}, \
            {"M1Ainv"}, \
		    {"M2en"}, \
		    {"M2src"}, \
		    {"M2dest"}, \
		    {"M2curv"}, \
		    {"M2scale"}, \
            {"M2Aen"}, \
            {"M2Asrc"}, \
            {"M2Aatt"}, \
            {"M2Acrv"}, \
            {"M2inv"}, \
            {"M2Ainv"}, \
		    {"M3en"}, \
		    {"M3src"}, \
		    {"M3dest"}, \
		    {"M3curv"}, \
		    {"M3scale"}, \
            {"M3Aen"}, \
            {"M3Asrc"}, \
            {"M3Aatt"}, \
            {"M3Acrv"}, \
            {"M3inv"}, \
            {"M3Ainv"}, \
		    {"M4en"}, \
		    {"M4src"}, \
		    {"M4dest"}, \
		    {"M4curv"}, \
		    {"M4scale"}, \
            {"M4Aen"}, \
            {"M4Asrc"}, \
            {"M4Aatt"}, \
            {"M4Acrv"}, \
            {"M4inv"}, \
            {"M4Ainv"}, \
		    {"M5en"}, \
		    {"M5src"}, \
		    {"M5dest"}, \
		    {"M5curv"}, \
		    {"M5scale"}, \
            {"M5Aen"}, \
            {"M5Asrc"}, \
            {"M5Aatt"}, \
            {"M5Acrv"}, \
            {"M5inv"}, \
            {"M5Ainv"}, \
		    {"M6en"}, \
		    {"M6src"}, \
		    {"M6dest"}, \
		    {"M6curv"}, \
		    {"M6scale"}, \
            {"M6Aen"}, \
            {"M6Asrc"}, \
            {"M6Aatt"}, \
            {"M6Acrv"}, \
            {"M6inv"}, \
            {"M6Ainv"}, \
		    {"M7en"}, \
		    {"M7src"}, \
		    {"M7dest"}, \
		    {"M7curv"}, \
		    {"M7scale"}, \
            {"M7Aen"}, \
            {"M7Asrc"}, \
            {"M7Aatt"}, \
            {"M7Acrv"}, \
            {"M7inv"}, \
            {"M7Ainv"}, \
		    {"M8en"}, \
		    {"M8src"}, \
		    {"M8dest"}, \
		    {"M8curv"}, \
		    {"M8scale"}, \
            {"M8Aen"}, \
            {"M8Asrc"}, \
            {"M8Aatt"}, \
            {"M8Acrv"}, \
            {"M8inv"}, \
            {"M8Ainv"}, \
            {"M9en"}, \
            {"M9src"}, \
            {"M9dest"}, \
            {"M9curv"}, \
            {"M9scale"}, \
            {"M9Aen"}, \
            {"M9Asrc"}, \
            {"M9Aatt"}, \
            {"M9Acrv"}, \
            {"M9inv"}, \
            {"M9Ainv"}, \
            {"M10en"}, \
            {"M10src"}, \
            {"M10dest"}, \
            {"M10curv"}, \
            {"M10scle"}, \
            {"M10Aen"}, \
            {"M10Asrc"}, \
            {"M10Aatt"}, \
            {"M10Acrv"}, \
            {"M10inv"}, \
            {"M10Ainv"}, \
            {"M11en"}, \
            {"M11src"}, \
            {"M11dest"}, \
            {"M11curv"}, \
            {"M11scle"}, \
            {"M11Aen"}, \
            {"M11Asrc"}, \
            {"M11Aatt"}, \
            {"M11Acrv"}, \
            {"M11inv"}, \
            {"M11Ainv"}, \
            {"M12en"}, \
            {"M12src"}, \
            {"M12dest"}, \
            {"M12curv"}, \
            {"M12scle"}, \
            {"M12Aen"}, \
            {"M12Asrc"}, \
            {"M12Aatt"}, \
            {"M12Acrv"}, \
            {"M12inv"}, \
            {"M12Ainv"}, \
            {"M13en"}, \
            {"M13src"}, \
            {"M13dest"}, \
            {"M13curv"}, \
            {"M13scle"}, \
            {"M13Aen"}, \
            {"M13Asrc"}, \
            {"M13Aatt"}, \
            {"M13Acrv"}, \
            {"M13inv"}, \
            {"M13Ainv"}, \
            {"M14en"}, \
            {"M14src"}, \
            {"M14dest"}, \
            {"M14curv"}, \
            {"M14scle"}, \
            {"M14Aen"}, \
            {"M14Asrc"}, \
            {"M14Aatt"}, \
            {"M14Acrv"}, \
            {"M14inv"}, \
            {"M14Ainv"}, \
            {"M15en"}, \
            {"M15src"}, \
            {"M15dest"}, \
            {"M15curv"}, \
            {"M15scle"}, \
            {"M15Aen"}, \
            {"M15Asrc"}, \
            {"M15Aatt"}, \
            {"M15Acrv"}, \
            {"M15inv"}, \
            {"M15Ainv"}, \
            {"M16en"}, \
            {"M16src"}, \
            {"M16dest"}, \
            {"M16curv"}, \
            {"M16scle"}, \
            {"M16Aen"}, \
            {"M16Asrc"}, \
            {"M16Aatt"}, \
            {"M16Acrv"}, \
            {"M16inv"}, \
            {"M16Ainv"}, \
            {"S1En"}, \
            {"S1Vol"}, \
            {"S1KRmin"}, \
            {"S1KRmax"}, \
            {"S1base"}, \
            {"S1LTrig"}, \
            {"S1Rev"}, \
            {"S1src"}, \
            {"S1gmidx"}, \
            {"S1strt"}, \
            {"S1LMode"}, \
            {"S1LSrc"}, \
            {"S1Lbeg"}, \
            {"S1Llen"}, \
            {"S1TunS"}, \
            {"S1TunF"}, \
            {"S1Frq"}, \
            {"S1FrqKT"}, \
            {"S1Intrp"}, \
            {"S1RelX"}, \
            {"S1AxMix"}, \
		    {"S1Edlt"}, \
		    {"S1Eatt"}, \
		    {"S1Eatc"}, \
		    {"S1Eht"}, \
		    {"S1Edt"}, \
		    {"S1Edc"}, \
		    {"S1Esl"}, \
		    {"S1Ert"}, \
		    {"S1Etc"}, \
		    {"S1Erst"}, \
            {"S2En"}, \
            {"S2Vol"}, \
            {"S2KRmin"}, \
            {"S2KRmax"}, \
            {"S2base"}, \
            {"S2LTrig"}, \
            {"S2Rev"}, \
            {"S2src"}, \
            {"S2gmidx"}, \
            {"S2strt"}, \
            {"S2LMode"}, \
            {"S2LSrc"}, \
            {"S2Lbeg"}, \
            {"S2Llen"}, \
            {"S2TunS"}, \
            {"S2TunF"}, \
            {"S2Frq"}, \
            {"S2FrqKT"}, \
            {"S2Intrp"}, \
            {"S2RelX"}, \
            {"S2AxMix"}, \
            {"S2Edlt"}, \
            {"S2Eatt"}, \
            {"S2Eatc"}, \
            {"S2Eht"}, \
            {"S2Edt"}, \
            {"S2Edc"}, \
            {"S2Esl"}, \
            {"S2Ert"}, \
            {"S2Etc"}, \
            {"S2Erst"}, \
            {"S3En"}, \
            {"S3Vol"}, \
            {"S3KRmin"}, \
            {"S3KRmax"}, \
            {"S3base"}, \
            {"S3LTrig"}, \
            {"S3Rev"}, \
            {"S3src"}, \
            {"S3gmidx"}, \
            {"S3strt"}, \
            {"S3LMode"}, \
            {"S3LSrc"}, \
            {"S3Lbeg"}, \
            {"S3Llen"}, \
            {"S3TunS"}, \
            {"S3TunF"}, \
            {"S3Frq"}, \
            {"S3FrqKT"}, \
            {"S3Intrp"}, \
            {"S3RelX"}, \
            {"S3AxMix"}, \
            {"S3Edlt"}, \
            {"S3Eatt"}, \
            {"S3Eatc"}, \
            {"S3Eht"}, \
            {"S3Edt"}, \
            {"S3Edc"}, \
            {"S3Esl"}, \
            {"S3Ert"}, \
            {"S3Etc"}, \
            {"S3Erst"}, \
            {"S4En"}, \
            {"S4Vol"}, \
            {"S4KRmin"}, \
            {"S4KRmax"}, \
            {"S4base"}, \
            {"S4LTrig"}, \
            {"S4Rev"}, \
            {"S4src"}, \
            {"S4gmidx"}, \
            {"S4strt"}, \
            {"S4LMode"}, \
            {"S4LSrc"}, \
            {"S4Lbeg"}, \
            {"S4Llen"}, \
            {"S4TunS"}, \
            {"S4TunF"}, \
            {"S4Frq"}, \
            {"S4FrqKT"}, \
            {"S4Intrp"}, \
            {"S4RelX"}, \
            {"S4AxMix"}, \
            {"S4Edlt"}, \
            {"S4Eatt"}, \
            {"S4Eatc"}, \
            {"S4Eht"}, \
            {"S4Edt"}, \
            {"S4Edc"}, \
            {"S4Esl"}, \
            {"S4Ert"}, \
            {"S4Etc"}, \
            {"S4Erst"}, \
        }

        enum class MainParamIndices : uint8_t
        {
            MasterVolume,
            VoicingMode,
            Unisono,
            OscillatorDetune,
            UnisonoDetune,
            OscillatorSpread,
            UnisonoStereoSpread,
            FMBrightness,

            AuxRouting,
            AuxWidth, // N11 to invert

            PortamentoTime,
            PortamentoCurve,
            PitchBendRange,

            Macro1,
            Macro2,
            Macro3,
            Macro4,
            Macro5,
            Macro6,
            Macro7,

            FMAmt2to1,
            FMAmt3to1,
            FMAmt4to1,
            FMAmt1to2,
            FMAmt3to2,
            FMAmt4to2,
            FMAmt1to3,
            FMAmt2to3,
            FMAmt4to3,
            FMAmt1to4,
            FMAmt2to4,
            FMAmt3to4,

            Count,
        };

        enum class SamplerParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Enabled,
            Volume,
            KeyRangeMin,
            KeyRangeMax,
            BaseNote,
            LegatoTrig,
            Reverse,
            SampleSource,
            GmDlsIndex,
            SampleStart,
            LoopMode,
            LoopSource,
            LoopStart,
            LoopLength,
            TuneSemis,
            TuneFine,
            FreqParam,
            FreqKT,
            InterpolationType,
            ReleaseExitsLoop,
            AuxMix,
            AmpEnvDelayTime,
            Count = AmpEnvDelayTime,
        };

        enum class ModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Enabled,
            Source,
            Destination,
            Curve,
            Scale,
            AuxEnabled,
            AuxSource,
            AuxAttenuation,
            AuxCurve,
            Invert,
            AuxInvert,
            Count,
        };
        enum class LFOParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Waveform,
            Waveshape,
            Restart,
            PhaseOffset,
            FrequencyParam,
            Sharpness,
            Count,
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
            Count,
        };
        enum class OscParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Enabled,
            Volume,
            KeyRangeMin,
            KeyRangeMax,
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
            AuxMix,
            AmpEnvDelayTime,
            Count = AmpEnvDelayTime,
        };

        enum class AuxParamIndexOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & FilterAuxParamIndexOffsets
        {
            Enabled,
            Link,
            Type,
            Param1, // filter type
            Param2, // filter Q
            Param3, // filter saturation
            Param4, // filter freq
            Param5, // filter KT
            Count,
        };

        // FILTER AUX INFO  ------------------------------------------------------------
        enum class FilterAuxParamIndexOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets & FilterAuxModIndexOffsets
        {
            unused__Enabled,
            unused__Link,
            unused__AuxType,
            FilterType, // filter type
            // FROM here, must be identical to FilterAuxModIndexOffsets (param2 - 5)
            Q, // filter Q
            Saturation, // filter saturation
            Freq, // filter freq
            FreqKT, // filter KT
            Count,
        };

        enum class FilterAuxModIndexOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets
        {
            Q, // filter Q
            Saturation, // filter saturation
            Freq, // filter freq
            FreqKT, // filter KT
            Count,
        };
        static_assert((int)FilterAuxParamIndexOffsets::Count == (int)AuxParamIndexOffsets::Count, "");
        #define FILTER_AUX_MOD_SUFFIXES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::FilterAuxModIndexOffsets::Count] { \
            " (Filter Q)", \
			" (Filter Saturation)", \
			" (Filter Frequency)", \
            " (n/a)", \
        }

//        // DISTORTION AUX INFO  ------------------------------------------------------------
//        enum class DistortionAuxParamIndexOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets & DistortionAuxModIndexOffsets
//        {
//            unused__Enabled,
//            unused__Link,
//            unused__AuxType,
//            DistortionStyle, // param 1
//            // FROM here, must be identical to DistortionAuxModIndexOffsets (param2 - 5)
//            Drive, // param 2
//            Threshold, // param 3
//            Shape, // param 4
//            OutputVolume, // param 5
//            Count,
//        };
//
//        enum class DistortionAuxModIndexOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets
//        {
//            Drive, // param 2
//            Threshold, // param 3
//            Shape, // param 4
//            OutputVolume, // param 5
//            Count,
//        };
//        static_assert((int)DistortionAuxParamIndexOffsets::Count == (int)AuxParamIndexOffsets::Count, "");
//#define DISTORTION_AUX_MOD_SUFFIXES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::DistortionAuxModIndexOffsets::Count] { \
//            " (Dist gain)", \
//			" (Dist threshold)", \
//			" (Dist shape)", \
//            " (n/a)", \
//        }
//

        // BITCRUSH AUX INFO  ------------------------------------------------------------
        enum class BitcrushAuxParamIndexOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets & BitcrushAuxModIndexOffsets
        {
            unused__Enabled,
            unused__Link,
            unused__AuxType,
            unused__1, // param 1
            // FROM here, must be identical to DistortionAuxModIndexOffsets (param2 - 5)
            unused__2, // param 2
            unused__3, // param 3
            Freq, // param 4
            FreqKT, // param 5
            Count,
        };

        enum class BitcrushAuxModIndexOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets
        {
            unused__2, // param 2
            unused__3, // param 3
            Freq, // param 4
            unused__5, // param 5
            Count,
        };
        static_assert((int)BitcrushAuxParamIndexOffsets::Count == (int)AuxParamIndexOffsets::Count, "");
#define BITCRUSH_AUX_MOD_SUFFIXES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::BitcrushAuxParamIndexOffsets::Count] { \
            " (n/a)", \
			" (n/a)", \
			" (Bitcrush frequency)", \
            " (n/a)", \
        }


        // ------------------------------------------------------------
        enum class AuxLink : uint8_t
        {
            // THESE ARE USED AS INDICES 0-3
            Aux1,
            Aux2,
            Aux3,
            Aux4,
            Count,
        };
        #define AUX_LINK_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::AuxLink::Count] { \
            "Aux 1", \
			"Aux 2", \
			"Aux 3", \
            "Aux 4", \
        }

        enum class AuxRoute : uint8_t
        {
            // A -- aux1 - aux2 --------------- L
            // B ---------------- aux3 - aux4 - R
            TwoTwo,

            // A -- aux1 - aux2 - aux3 -------- L
            // B ----------------------- aux4 - R
            ThreeOne,

            // A -- aux1 - aux2 - aux3 - aux4 - L
            // B -------------------------------R
            FourZero,

            // A -- aux1 - aux2 - aux3 - aux4 - L
            // B -^                             R
            SerialMono,

            Count,
        };
        #define AUX_ROUTE_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::AuxRoute::Count] { \
            "TwoTwo", \
			"ThreeOne", \
			"FourZero", \
			"Serial (mono)", \
        }

        enum class AuxEffectType : uint8_t
        {
            None,
            BigFilter,
            //Distortion,
            Bitcrush,
            Count,
        };
        #define AUX_EFFECT_TYPE_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::AuxEffectType::Count] { \
            "None", \
			"BigFilter", \
			"Bitcrush", \
        }

        struct IAuxEffect
        {
            virtual ~IAuxEffect() {}
            virtual void AuxBeginBlock(float noteHz, int nSamples, struct ModMatrixNode& modMatrix) = 0;
            virtual float AuxProcessSample(float inp) = 0;
        };

        enum FMMatrixIndices
        {
            FMAmt2to1 = 0,
            FMAmt3to1 = 1,
            FMAmt4to1 = 2,
            FMAmt1to2 = 3,
            FMAmt3to2 = 4,
            FMAmt4to2 = 5,
            FMAmt1to3 = 6,
            FMAmt2to3 = 7,
            FMAmt4to3 = 8,
            FMAmt1to4 = 9,
            FMAmt2to4 = 10,
            FMAmt3to4 = 11,
        };

    } // namespace M7


} // namespace WaveSabreCore








