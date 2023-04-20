
#pragma once

#include <Windows.h>
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
    class CriticalSection
    {
    public:
        class CriticalSectionGuard
        {
        public:
            CriticalSectionGuard(CriticalSection* criticalSection)
                : criticalSection(criticalSection)
            {
                EnterCriticalSection(&criticalSection->criticalSection);
            }
            ~CriticalSectionGuard()
            {
                LeaveCriticalSection(&criticalSection->criticalSection);
            }

        private:
            CriticalSection* criticalSection;
        };

        CriticalSection()
        {
            InitializeCriticalSection(&criticalSection);
        }
        ~CriticalSection()
        {
            DeleteCriticalSection(&criticalSection);
        }

        CriticalSectionGuard Enter()
        {
            return CriticalSectionGuard(this);
        }

        void ManualEnter()
        {
            EnterCriticalSection(&criticalSection);
        }
        void ManualLeave()
        {
            LeaveCriticalSection(&criticalSection);
        }

    private:
        CRITICAL_SECTION criticalSection;
    };

    namespace M7
    {
        void Init();

        using real_t = float;

        template<typename Tfirst, typename Tsecond>
        struct Pair
        {
            Tfirst first;
            Tsecond second;
        };

        using FloatPair = Pair<float, float>;

        struct FreqParamConfig
        {
            const float mCenterFrequency;
            const float mScale;
            const float mCenterMidiNote;

            // calculate center midi note with this js in browser console:
            // let hztomidi = function(hz) { return 12.0 * Math.log2(Math.max(8.0, hz) / 440) + 69; }
            // hztomidi(1000)
            // = 83.21309485364912

            // i would prefer a compile-time version but whatev
            constexpr FreqParamConfig(float centerFrequency, float scale, float centerMidiNote) :
                mCenterFrequency(centerFrequency),
                mScale(scale),
                mCenterMidiNote(centerMidiNote)
            {
            }
        };

        struct IntParamConfig
        {
            const int mMinValInclusive;
            const int mMaxValInclusive;
            constexpr int GetDiscreteValueCount() const
            {
                return mMaxValInclusive - mMinValInclusive + 1;
            }
        };

        struct VolumeParamConfig
        {
            const float mMaxValLinear;
            const float mMaxValDb;
            // plug into a browser console to convert around
            //let todb = function(aLinearValue, aMinDecibels) {
            //    const LOG10E = Math.LOG10E || Math.log(10); // Constant value of log base 10 of e
            //    const decibels = 20 * Math.log10(aLinearValue); // Calculate decibels using logarithmic function
            //    return (decibels != = -Infinity) ? decibels : aMinDecibels; // Check for infinite value and return the result or minimum decibels
            //}
            //let tolinear = function(aDecibelValue) {
            //    return Math.pow(10, aDecibelValue / 20);
            //}
        };

        extern bool gAlwaysTrue;

        static constexpr real_t FloatEpsilon = 0.000001f;
        static constexpr float MIN_DECIBEL_GAIN = -60.0f;

        static constexpr FreqParamConfig gFilterFreqConfig{ 1000, 10, 83.21309485364912f };
        //static constexpr real_t gFilterCenterFrequency = 1000.0f;
        //static constexpr real_t gFilterFrequencyScale = 10.0f;
        static constexpr real_t gFreqParamKTUnity = 0.3f;
        static constexpr size_t gModulationSpecDestinationCount = 4;

        static constexpr FreqParamConfig gLFOLPFreqConfig{ 20, 7, 15.486820576352429f };
        //static constexpr real_t gLFOLPCenterFrequency = 20.0f;
        //static constexpr real_t gLFOLPFrequencyScale = 7.0f;

        static constexpr FreqParamConfig gBitcrushFreqConfig{ gFilterFreqConfig };
        //        static constexpr float gBitcrushFreqCenterFreq = 1000;
        //static constexpr float gBitcrushFreqRange = 10;

        static constexpr FreqParamConfig gSourceFreqConfig{ gFilterFreqConfig };

        //static constexpr real_t gSourceFrequencyCenterHz = 1000;
        //static constexpr real_t gSourceFrequencyScale = 10;

        static constexpr FreqParamConfig gLFOFreqConfig{ 1.5f, 8, -0.37631656229592636f }; // well midi note here is meaningless and i hope you never use it.

        //static constexpr real_t gLFOFrequencyCenterHz = 1.5f;
        //static constexpr real_t gLFOFrequencyScale = 8;

        static constexpr FreqParamConfig gSyncFreqConfig{ gFilterFreqConfig };

        static constexpr IntParamConfig gSourcePitchSemisRange{ -36, 36 };
        static constexpr IntParamConfig gKeyRangeCfg { 0, 127 };
        //static constexpr int gSourcePitchSemisRange = 36;
        static constexpr float gSourcePitchFineRangeSemis = 2;

        static constexpr int gPitchBendMaxRange = 24;
        static constexpr int gUnisonoVoiceMax = 12;

        static constexpr IntParamConfig gPitchBendCfg{ -gPitchBendMaxRange,gPitchBendMaxRange };
        static constexpr IntParamConfig gUnisonoVoiceCfg { 1, gUnisonoVoiceMax };

        static constexpr int gMaxMaxVoices = 64;
        static constexpr IntParamConfig gMaxVoicesCfg{ 1, gMaxMaxVoices };

        static constexpr int gGmDlsSampleCount = 495;
        static constexpr IntParamConfig gGmDlsIndexParamCfg { -1, gGmDlsSampleCount };
        //-1, WaveSabreCore::GmDls::NumSamples


        //static constexpr real_t gSyncFrequencyCenterHz = 1000;
        //static constexpr real_t gSyncFrequencyScale = 10;
        static constexpr real_t gFrequencyMulMax = 64;


        //static constexpr real_t gMasterVolumeMaxDb = 6;
        static constexpr VolumeParamConfig gMasterVolumeCfg{ 1.9952623149688795f, 6.0f };
        static constexpr VolumeParamConfig gUnityVolumeCfg{ 1, 0 };




        static constexpr size_t gModulationCount = 18;

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

        static constexpr uint16_t gAudioRecalcSampleMaskValues[] = {
            127, // Potato,
            63, // Carrot,
            31, // Cauliflower,
            7, // Celery,
            1, // Artichoke,
        };

        static constexpr uint16_t gModulationRecalcSampleMaskValues[] = {
            127, // Potato,
            63, // Carrot,
            31, // Cauliflower,
            15, // Celery,
            7, // Artichoke,
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
            static constexpr float gSqrt2 = 1.41421356237f;
            static constexpr float gSqrt2Recip = 0.70710678118f;

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
            inline constexpr real_t clamp01(real_t x)
            {
                return clamp(x, 0, 1);
            }
            inline constexpr real_t clampN11(real_t x)
            {
                return clamp(x, -1, 1);
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
            inline float map_clamped(float inp, float inp_min, float inp_max, float out_min, float out_max)
            {
                float t = lerp_rev(inp_min, inp_max, inp);
                return lerp(out_min, out_max, clamp01(t));
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

            inline void* GetCrtProc(const char * const imp)
            {
#ifdef MIN_SIZE_REL
                HANDLE h = GetModuleHandleA("msvcrt");
#else
                HANDLE h = GetModuleHandleA("ucrtbase");
#endif
                return GetProcAddress((HMODULE)h, imp);
            }

            struct CrtFns {
                CrtFns() : 
                    crt_sin(decltype(crt_sin)(GetCrtProc("sin"))),
                    crt_cos(decltype(crt_cos)(GetCrtProc("cos"))),
                    crt_tan(decltype(crt_tan)(GetCrtProc("tan"))),
                    crt_floor(decltype(crt_floor)(GetCrtProc("floor"))),
                    crt_pow(decltype(crt_pow)(GetCrtProc("pow"))),
                    crt_log(decltype(crt_log)(GetCrtProc("log")))
                {
                    //
                }
                double(__cdecl* crt_sin)(double);
                double(__cdecl* crt_cos)(double);
                double(__cdecl* crt_tan)(double);
                double(__cdecl* crt_floor)(double);
                double(__cdecl* crt_pow)(double, double);
                double(__cdecl* crt_log)(double);
            };

            extern CrtFns* gCrtFns;

            inline double CrtSin(double x) {
                return gCrtFns->crt_sin(x);
            }

            inline double CrtCos(double x) {
                return gCrtFns->crt_cos(x);
            }

            inline double CrtLog(double x) {
                return gCrtFns->crt_log(x);
            }

            inline double CrtFloor(double x) {
                return gCrtFns->crt_floor(x);
            }

            inline double CrtPow(double x, double y) {
                return gCrtFns->crt_pow(x, y);
            }

            inline double CrtTan(double x) {
                return gCrtFns->crt_tan(x);
            }

            // MSVC has weird rules about implicitly calling CRT functions to cast between floats & integers. this encapsulates the chaos.
            inline long DoubleToLong(double x) {
                return (long)x;
            }

            // MSVC has weird rules about implicitly calling CRT functions to cast between floats & integers. this encapsulates the chaos.
            inline long FloatToLong(float x) {
                return (long)x;
            }

            inline real_t pow(real_t x, real_t y) {
                // fasterpow is just too inaccurate.
                //return fastmath::fastpow(x, y); // this is also not always accurate, just due to ranges. 
                return (float)CrtPow((double)x, (double)y);
            }
            inline real_t pow2(real_t y) {
                return (float)CrtPow(2, (double)y);
            }

            //inline double LOG(double x) {
            //    //static CrtMathFn gLog{ "log" };
            //    //return gLog.invoke(x);
            //    //static double(__cdecl * log_fn_t)(double) = nullptr;
            //    //if (!log_fn_t) {
            //    //    log_fn_t = (decltype(log_fn_t))GetProcAddress(GetModuleHandleA("msvcrt"), "log");
            //    //}
            //    //return log_fn_t(x);
            //    //return 0;
            //    //return ::logl(x);
            //}
            inline float log2(float x) {
                static constexpr float log2_e = 1.44269504089f; // 1/log(2)
                //return (float)::logl((double)x) * log2_e;
                return (float)CrtLog((double)x) * log2_e;
                //return 0;
            }
            inline float log10(float x) {
                static constexpr float log10_e = 0.4342944819f;// 1.0f / LOG(10.0f);
                return (float)CrtLog((double)x) * log10_e;
                //return 0;
            }
            inline real_t tan(real_t x) {
                return (float)CrtTan((double)x);// fastmath::fastertanfull(x); // less fast to call lib function, but smaller code.
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

            // pow(2,n) is a quite hot path, used by MidiNoteToFrequency(), as well as all other frequency calculations.
            // the range of `n` is [-15,+15] but depends on frequency param scale. so let's extend a bit and make a huge lut.
            struct Pow2_N16_16_LUT : public LUT01
            {
                Pow2_N16_16_LUT(size_t nSamples);
                virtual float Invoke(float x) const override;
            };
            struct LUTs
            {
                SinCosLUT gSinLUT;
                SinCosLUT gCosLUT;
                TanHLUT gTanhLUT;
                LUT01 gSqrt01LUT;// sqrt 01

                CurveLUT gCurveLUT;
                Pow2_N16_16_LUT gPow2_N16_16_LUT;
                // pow10 can be useful too.

                LUTs();
            };

            extern LUTs* gLuts;

            inline float pow2_N16_16(float n)
            {
                return gLuts->gPow2_N16_16_LUT.Invoke(n);
            }

            inline real_t sin(real_t x) {
                return gLuts->gSinLUT.Invoke(x);
                //return (real_t)Helpers::FastSin((double)x);
            }
            inline real_t cos(real_t x) {
                return gLuts->gCosLUT.Invoke(x);// (real_t)Helpers::FastCos((double)x);
            }

            inline real_t sqrt(real_t x) {
                return (float)::sqrt((double)x);
            }

            // optimized LUT works for 0<=x<=1
            inline real_t sqrt01(real_t x) {
                return gLuts->gSqrt01LUT.Invoke(x);
            }

            inline real_t tanh(real_t x) {
                return gLuts->gTanhLUT.Invoke(x);
                //return (float)::tanh((double)x);
                //return fastmath::fastertanh(x); // tanh is used for saturation and the fast version adds weird high frequency content. try it on a LP filter and see what i mean.
            }

            inline real_t modCurve_xN11_kN11(real_t x, real_t k)
            {
                return gLuts->gCurveLUT.Invoke(x, k);
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
            FloatPair PanToLRVolumeParams(float panN11);

            FloatPair PanToFactor(float panN11);


            inline float Sample16To32Bit(int16_t s)
            {
                return clampN11(float(s) / 32768);
            }

            inline int16_t Sample32To16(float f)
            {
                return int16_t(ClampI(int32_t(f * 32768), -32768, 32767));
            }

        } // namespace math


        inline void MSEncode(float left, float right, float* mid, float* side) {
            *mid = (left + right) * math::gSqrt2Recip;
            *side = (left - right) * math::gSqrt2Recip;
        }

        inline void MSDecode(float mid, float side, float* left, float* right) {
            *left = (mid + side) * math::gSqrt2Recip;
            *right = (mid - side) * math::gSqrt2Recip;
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
        void BipolarDistribute(size_t count, const bool* enabled, float* outp);

        void ImportDefaultsArray(size_t count, const int16_t* src, float* paramCacheOffset);

        enum class TimeBasis : uint8_t
        {
            Frequency,
            Time,
            Beats,
            Count,
        };
#define TIME_BASIS_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::TimeBasis::Count]{ \
                "Frequency", \
                "Time", \
                "Beats", \
        };

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
            static constexpr real_t gMinRawVal = 12; // slightly above the correct value (~1ms), in order to guarantee envelope times can reach true 0.
                //(1.0f / (1 << (gRangeLog2 / 2))) *
                //gCenterValue; // minimum value as described by log calc (before subtracting min).
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
            IntParamConfig mCfg;
            //const int mMinValueInclusive;
            //const int mMaxValueInclusive;
            //const float mHalfMinusMinVal; // precalc to avoid an add
            int mCachedVal;
            //explicit IntParam(real_t& ref, int minValueInclusive, int maxValueInclusive, int initialValue);
            explicit IntParam(real_t& ref, const IntParamConfig& cfg);
//            int GetDiscreteValueCount() const;
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
            static constexpr IntParamConfig gEnumIntCfg{ 0, MaxItems };
            T mCachedVal;
            explicit EnumParam(real_t& ref, T maxValue) : IntParam(ref, gEnumIntCfg)
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
            //const float mMaxVolumeLinearGain;// 1.122f = +1 db.
            VolumeParamConfig mCfg;

            float ParamToLinear(float x) const;
            float LinearToParam(float x) const;
            float ParamToDecibels(float x) const;
            float DecibelsToParam(float db) const;

        public:
            explicit VolumeParam(real_t& ref, const VolumeParamConfig& cfg);
            float GetLinearGain(float modVal = 0.0f) const;
            float GetDecibels() const;
            bool IsSilent() const;
            void SetLinearValue(float f);
            void SetDecibels(float db);
        };

        // value 0.3 = unity, and each 0.1 param value = 1 octave transposition, when KT = 1.
        // when KT = 0, 0.5 = 1khz, and each 0.1 param value = +/- octave.
        struct FrequencyParam
        {
            const FreqParamConfig mCfg;

        public:
            Float01Param mValue;
            Float01Param mKTValue; // how much key tracking to apply. when 0, the frequency doesn't change based on playing note. when 1, it scales completely with the note's frequency.

            //explicit FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/, real_t initialValue, real_t initialKT);
            //explicit FrequencyParam(real_t& valRef, real_t& ktRef, real_t centerFrequency, real_t scale/*=10.0f*/);
            explicit FrequencyParam(real_t& valRef, real_t& ktRef, const FreqParamConfig& mCfg);
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
                return math::clamp01(mpArray[i]);
            }
            float Get01Value(size_t i, float mod) const {
                return math::clamp01(mpArray[i] + mod);
            }
        };


        struct Serializer
        {
            uint8_t* mBuffer = nullptr;
            size_t mAllocatedSize = 0;
            size_t mSize = 0;

            ~Serializer();

            static void FreeBuffer(void* p) {
                delete[] p;
            }

            Pair<uint8_t*, size_t> DetachBuffer();
            uint8_t* GrowBy(size_t n);
            void WriteUByte(uint8_t b);

            void WriteFloat(float f);
            void WriteUInt32(uint32_t f);
            void WriteBuffer(const uint8_t* buf, size_t n);

            void WriteInt16NormalizedFloat(float f) {
                auto p = (int16_t*)(GrowBy(sizeof(int16_t)));
                *p = math::Sample32To16(f);
                //*p = int16_t(math::ClampInclusive(int32_t(f * 32768), -32768, 32767));
            }
        };

        // attaches to some buffer
        struct Deserializer
        {
            const uint8_t* mpData;
            const uint8_t* mpCursor;
            //const uint8_t* mpEnd;
            //size_t mSize;
            explicit Deserializer(const uint8_t* p);
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

            uint32_t ReadVarUInt32()
            {
                uint32_t value = 0;
                uint32_t shift = 0;
                // byte stream: [c-------]...
                uint8_t b = 0;
                do {
                    b = ReadUByte();
                    value |= static_cast<uint32_t>(b & 0x7f) << shift;
                    shift += 7;
                } while ((b & 0x80) != 0);
                return value;
            }

            float ReadFloat();
            double ReadDouble();
            // returns a new cursor in the out buffer 
            void ReadBuffer(void* out, size_t numbytes);

            float ReadInt16NormalizedFloat() {
                int16_t ret = *((int16_t*)mpCursor);
                mpCursor += sizeof(ret);
                return math::Sample16To32Bit(ret);
                //return math::clampN11(float(ret) / 32768);
            }

            //ByteBitfield ReadByteBitfield() {
            //    return ByteBitfield{ ReadUByte() };
            //}
            //WordBitfield ReadWordBitfield() {
            //    return WordBitfield{ ReadUWord() };
            //}
        };


        // you need to sync up all these enums below to correspond with ordering et al.
        enum class ParamIndices : uint16_t
        {
            MasterVolume,
            VoicingMode,
            Unisono,
            OscillatorDetune,
            UnisonoDetune,
            OscillatorSpread,
            UnisonoStereoSpread,
            FMBrightness,

            PortamentoTime,
            PortamentoCurve,
            PitchBendRange,

            MaxVoices,

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
            Osc1AmpEnvMode,

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
                Osc2AmpEnvMode,

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
                Osc3AmpEnvMode,

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
                Osc4AmpEnvMode,

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
                Env1AmpEnvMode,

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
                Env2AmpEnvMode,

            LFO1Waveform, // KEEP IN SYNC WITH LFOParamIndexOffsets
            LFO1Waveshape,
            LFO1Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
            LFO1PhaseOffset,
                LFO1FrequencyBasis,
            LFO1FrequencyParam,
            LFO1Sharpness,

                LFO2Waveform, // KEEP IN SYNC WITH LFOParamIndexOffsets
                LFO2Waveshape,
                LFO2Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
                LFO2PhaseOffset,
                    LFO2FrequencyBasis,
                    LFO2FrequencyParam,
                LFO2Sharpness,

                LFO3Waveform, // KEEP IN SYNC WITH LFOParamIndexOffsets
                LFO3Waveshape,
                LFO3Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
                LFO3PhaseOffset,
                LFO3FrequencyBasis,
                LFO3FrequencyParam,
                LFO3Sharpness,

                LFO4Waveform, // KEEP IN SYNC WITH LFOParamIndexOffsets
                LFO4Waveshape,
                LFO4Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
                LFO4PhaseOffset,
                LFO4FrequencyBasis,
                LFO4FrequencyParam,
                LFO4Sharpness,

                Filter1Enabled,
                Filter1Type, // filter type
                Filter1Q,
                Filter1Freq,
                Filter1FreqKT,

                    Filter2Enabled,
                Filter2Type, // filter type
                Filter2Q,
                    Filter2Freq,
                    Filter2FreqKT,

            Mod1Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod1Source,
                    Mod1Destination1,
                    Mod1Destination2,
                    Mod1Destination3,
                    Mod1Destination4,
                    Mod1Curve,
                    Mod1Scale1,
                    Mod1Scale2,
                    Mod1Scale3,
                    Mod1Scale4,
                    Mod1AuxEnabled,
            Mod1AuxSource,
            Mod1AuxAttenuation,
            Mod1AuxCurve,
                Mod1SrcRangeMin,
                Mod1SrcRangeMax,
                Mod1AuxRangeMin,
                Mod1AuxRangeMax,

            Mod2Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod2Source,
                Mod2Destination1,
                Mod2Destination2,
                Mod2Destination3,
                Mod2Destination4,
                Mod2Curve,
                Mod2Scale1,
                Mod2Scale2,
                Mod2Scale3,
                Mod2Scale4,
                Mod2AuxEnabled,
            Mod2AuxSource,
            Mod2AuxAttenuation,
            Mod2AuxCurve,
                Mod2SrcRangeMin,
                Mod2SrcRangeMax,
                Mod2AuxRangeMin,
                Mod2AuxRangeMax,

            Mod3Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod3Source,
                Mod3Destination1,
                Mod3Destination2,
                Mod3Destination3,
                Mod3Destination4,
                Mod3Curve,
                    Mod3Scale1,
                    Mod3Scale2,
                    Mod3Scale3,
                    Mod3Scale4,
                    Mod3AuxEnabled,
            Mod3AuxSource,
            Mod3AuxAttenuation,
            Mod3AuxCurve,
                    Mod3SrcRangeMin,
                    Mod3SrcRangeMax,
                    Mod3AuxRangeMin,
                    Mod3AuxRangeMax,

            Mod4Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod4Source,
                    Mod4Destination1,
                    Mod4Destination2,
                    Mod4Destination3,
                    Mod4Destination4,
                    Mod4Curve,
                    Mod4Scale1,
                    Mod4Scale2,
                    Mod4Scale3,
                    Mod4Scale4,
                    Mod4AuxEnabled,
            Mod4AuxSource,
            Mod4AuxAttenuation,
            Mod4AuxCurve,
                    Mod4SrcRangeMin,
                    Mod4SrcRangeMax,
                    Mod4AuxRangeMin,
                    Mod4AuxRangeMax,

            Mod5Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod5Source,
                    Mod5Destination1,
                    Mod5Destination2,
                    Mod5Destination3,
                    Mod5Destination4,
                    Mod5Curve,
                    Mod5Scale1,
                    Mod5Scale2,
                    Mod5Scale3,
                    Mod5Scale4,
                    Mod5AuxEnabled,
            Mod5AuxSource,
            Mod5AuxAttenuation,
            Mod5AuxCurve,
                    Mod5SrcRangeMin,
                    Mod5SrcRangeMax,
                    Mod5AuxRangeMin,
                    Mod5AuxRangeMax,

            Mod6Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod6Source,
                    Mod6Destination1,
                    Mod6Destination2,
                    Mod6Destination3,
                    Mod6Destination4,
                    Mod6Curve,
                    Mod6Scale1,
                    Mod6Scale2,
                    Mod6Scale3,
                    Mod6Scale4,
                    Mod6AuxEnabled,
            Mod6AuxSource,
            Mod6AuxAttenuation,
            Mod6AuxCurve,
                    Mod6SrcRangeMin,
                    Mod6SrcRangeMax,
                    Mod6AuxRangeMin,
                    Mod6AuxRangeMax,

            Mod7Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod7Source,
                    Mod7Destination1,
                    Mod7Destination2,
                    Mod7Destination3,
                    Mod7Destination4,
                    Mod7Curve,
                Mod7Scale1,
                Mod7Scale2,
                Mod7Scale3,
                Mod7Scale4,
                Mod7AuxEnabled,
            Mod7AuxSource,
            Mod7AuxAttenuation,
            Mod7AuxCurve,
                    Mod7SrcRangeMin,
                    Mod7SrcRangeMax,
                    Mod7AuxRangeMin,
                    Mod7AuxRangeMax,

            Mod8Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
            Mod8Source,
                Mod8Destination1,
                Mod8Destination2,
                Mod8Destination3,
                Mod8Destination4,
                Mod8Curve,
                    Mod8Scale1,
                    Mod8Scale2,
                    Mod8Scale3,
                    Mod8Scale4,
                    Mod8AuxEnabled,
            Mod8AuxSource,
            Mod8AuxAttenuation,
            Mod8AuxCurve,
                        Mod8SrcRangeMin,
                        Mod8SrcRangeMax,
                        Mod8AuxRangeMin,
                        Mod8AuxRangeMax,

                Mod9Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod9Source,
                Mod9Destination1,
                Mod9Destination2,
                Mod9Destination3,
                Mod9Destination4,
                Mod9Curve,
                    Mod9Scale1,
                    Mod9Scale2,
                    Mod9Scale3,
                    Mod9Scale4,
                    Mod9AuxEnabled,
                Mod9AuxSource,
                Mod9AuxAttenuation,
                Mod9AuxCurve,
                        Mod9SrcRangeMin,
                        Mod9SrcRangeMax,
                        Mod9AuxRangeMin,
                        Mod9AuxRangeMax,

                Mod10Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod10Source,
                Mod10Destination1,
                Mod10Destination2,
                Mod10Destination3,
                Mod10Destination4,
                Mod10Curve,
                    Mod10Scale1,
                    Mod10Scale2,
                    Mod10Scale3,
                    Mod10Scale4,
                    Mod10AuxEnabled,
                Mod10AuxSource,
                Mod10AuxAttenuation,
                Mod10AuxCurve,
                        Mod10SrcRangeMin,
                        Mod10SrcRangeMax,
                        Mod10AuxRangeMin,
                        Mod10AuxRangeMax,

                Mod11Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod11Source,
                Mod11Destination1,
                Mod11Destination2,
                Mod11Destination3,
                Mod11Destination4,
                Mod11Curve,
                    Mod11Scale1,
                    Mod11Scale2,
                    Mod11Scale3,
                    Mod11Scale4,
                    Mod11AuxEnabled,
                Mod11AuxSource,
                Mod11AuxAttenuation,
                Mod11AuxCurve,
                        Mod11SrcRangeMin,
                        Mod11SrcRangeMax,
                        Mod11AuxRangeMin,
                        Mod11AuxRangeMax,

                Mod12Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod12Source,
                Mod12Destination1,
                Mod12Destination2,
                Mod12Destination3,
                Mod12Destination4,
                Mod12Curve,
                    Mod12Scale1,
                    Mod12Scale2,
                    Mod12Scale3,
                    Mod12Scale4,
                    Mod12AuxEnabled,
                Mod12AuxSource,
                Mod12AuxAttenuation,
                Mod12AuxCurve,
                        Mod12SrcRangeMin,
                        Mod12SrcRangeMax,
                        Mod12AuxRangeMin,
                        Mod12AuxRangeMax,

                Mod13Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod13Source,
                Mod13Destination1,
                Mod13Destination2,
                Mod13Destination3,
                Mod13Destination4,
                Mod13Curve,
                    Mod13Scale1,
                    Mod13Scale2,
                    Mod13Scale3,
                    Mod13Scale4,
                    Mod13AuxEnabled,
                Mod13AuxSource,
                Mod13AuxAttenuation,
                Mod13AuxCurve,
                            Mod13SrcRangeMin,
                            Mod13SrcRangeMax,
                            Mod13AuxRangeMin,
                            Mod13AuxRangeMax,

                Mod14Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod14Source,
                Mod14Destination1,
                Mod14Destination2,
                Mod14Destination3,
                Mod14Destination4,
                Mod14Curve,
                    Mod14Scale1,
                    Mod14Scale2,
                    Mod14Scale3,
                    Mod14Scale4,
                    Mod14AuxEnabled,
                Mod14AuxSource,
                Mod14AuxAttenuation,
                Mod14AuxCurve,
                            Mod14SrcRangeMin,
                            Mod14SrcRangeMax,
                            Mod14AuxRangeMin,
                            Mod14AuxRangeMax,

                Mod15Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod15Source,
                Mod15Destination1,
                Mod15Destination2,
                Mod15Destination3,
                Mod15Destination4,
                Mod15Curve,
                    Mod15Scale1,
                    Mod15Scale2,
                    Mod15Scale3,
                    Mod15Scale4,
                    Mod15AuxEnabled,
                Mod15AuxSource,
                Mod15AuxAttenuation,
                Mod15AuxCurve,
                            Mod15SrcRangeMin,
                            Mod15SrcRangeMax,
                            Mod15AuxRangeMin,
                            Mod15AuxRangeMax,

                Mod16Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod16Source,
                    Mod16Destination1,
                    Mod16Destination2,
                    Mod16Destination3,
                    Mod16Destination4,
                    Mod16Curve,
                    Mod16Scale1,
                    Mod16Scale2,
                    Mod16Scale3,
                    Mod16Scale4,
                    Mod16AuxEnabled,
                Mod16AuxSource,
                Mod16AuxAttenuation,
                Mod16AuxCurve,
                            Mod16SrcRangeMin,
                            Mod16SrcRangeMax,
                            Mod16AuxRangeMin,
                            Mod16AuxRangeMax,

                Mod17Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                Mod17Source,
                    Mod17Destination1,
                    Mod17Destination2,
                    Mod17Destination3,
                    Mod17Destination4,
                    Mod17Curve,
                    Mod17Scale1,
                    Mod17Scale2,
                    Mod17Scale3,
                    Mod17Scale4,
                    Mod17AuxEnabled,
                Mod17AuxSource,
                Mod17AuxAttenuation,
                Mod17AuxCurve,
                            Mod17SrcRangeMin,
                            Mod17SrcRangeMax,
                            Mod17AuxRangeMin,
                            Mod17AuxRangeMax,

                    Mod18Enabled, // KEEP IN SYNC WITH ModParamIndexOffsets
                    Mod18Source,
                    Mod18Destination1,
                    Mod18Destination2,
                    Mod18Destination3,
                    Mod18Destination4,
                    Mod18Curve,
                    Mod18Scale1,
                    Mod18Scale2,
                    Mod18Scale3,
                    Mod18Scale4,
                    Mod18AuxEnabled,
                    Mod18AuxSource,
                    Mod18AuxAttenuation,
                    Mod18AuxCurve,
                                Mod18SrcRangeMin,
                                Mod18SrcRangeMax,
                                Mod18AuxRangeMin,
                                Mod18AuxRangeMax,

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
                    Sampler1Delay,

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
                    Sampler1AmpEnvMode,

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
                                Sampler2Delay,

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
                    Sampler2AmpEnvMode,

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
                        Sampler3Delay,

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
                    Sampler3AmpEnvMode,

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
                    Sampler4Delay,

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
                        Sampler4AmpEnvMode,

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
{"PortTm"}, \
{"PortCv"}, \
{"PBRng"}, \
{"MaxVox"}, \
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
{"AE1mode"}, \
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
{"AE2mode"}, \
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
{"AE3mode"}, \
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
{"AE4mode"}, \
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
{"E1mode"}, \
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
{"E2mode"}, \
{"LFO1wav"}, \
{"LFO1shp"}, \
{"LFO1rst"}, \
{"LFO1ph"}, \
{"LFO1bas"}, \
{"LFO1fr"}, \
{"LFO1lp"}, \
{"LFO2wav"}, \
{"LFO2shp"}, \
{"LFO2rst"}, \
{"LFO2ph"}, \
{"LFO2bas"}, \
{"LFO2fr"}, \
{"LFO2lp"}, \
{"LFO3wav"}, \
{"LFO3shp"}, \
{"LFO3rst"}, \
{"LFO3ph"}, \
{"LFO3bas"}, \
{"LFO3fr"}, \
{"LFO3lp"}, \
{"LFO4wav"}, \
{"LFO4shp"}, \
{"LFO4rst"}, \
{"LFO4ph"}, \
{"LFO4bas"}, \
{"LFO4fr"}, \
{"LFO4lp"}, \
{"F1En"}, \
{"F1Type"}, \
{"F1Q"}, \
{"F1Freq"}, \
{"F1FKT"}, \
{"F2En"}, \
{"F2Type"}, \
{"F2Q"}, \
{"F2Freq"}, \
{"F2FKT"}, \
{"M1en"}, \
{"M1src"}, \
{"M1dest1"}, \
{"M1dest2"}, \
{"M1dest3"}, \
{"M1dest4"}, \
{"M1curv"}, \
{"M1scl1"}, \
{"M1scl2"}, \
{"M1scl3"}, \
{"M1scl4"}, \
{"M1Aen"}, \
{"M1Asrc"}, \
{"M1Aatt"}, \
{"M1Acrv"}, \
{"M1rngA"}, \
{"M1rngB"}, \
{"M1rngXA"}, \
{"M1rngXB"}, \
{"M2en"}, \
{"M2src"}, \
{"M2dest1"}, \
{"M2dest2"}, \
{"M2dest3"}, \
{"M2dest4"}, \
{"M2curv"}, \
{"M2scl1"}, \
{"M2scl2"}, \
{"M2scl3"}, \
{"M2scl4"}, \
{"M2Aen"}, \
{"M2Asrc"}, \
{"M2Aatt"}, \
{"M2Acrv"}, \
{"M2rngA"}, \
{"M2rngB"}, \
{"M2rngXA"}, \
{"M2rngXB"}, \
{"M3en"}, \
{"M3src"}, \
{"M3dest1"}, \
{"M3dest2"}, \
{"M3dest3"}, \
{"M3dest4"}, \
{"M3curv"}, \
{"M3scl1"}, \
{"M3scl2"}, \
{"M3scl3"}, \
{"M3scl4"}, \
{"M3Aen"}, \
{"M3Asrc"}, \
{"M3Aatt"}, \
{"M3Acrv"}, \
{"M3rngA"}, \
{"M3rngB"}, \
{"M3rngXA"}, \
{"M3rngXB"}, \
{"M4en"}, \
{"M4src"}, \
{"M4dest1"}, \
{"M4dest2"}, \
{"M4dest3"}, \
{"M4dest4"}, \
{"M4curv"}, \
{"M4scl1"}, \
{"M4scl2"}, \
{"M4scl3"}, \
{"M4scl4"}, \
{"M4Aen"}, \
{"M4Asrc"}, \
{"M4Aatt"}, \
{"M4Acrv"}, \
{"M4rngA"}, \
{"M4rngB"}, \
{"M4rngXA"}, \
{"M4rngXB"}, \
{"M5en"}, \
{"M5src"}, \
{"M5dest1"}, \
{"M5dest2"}, \
{"M5dest3"}, \
{"M5dest4"}, \
{"M5curv"}, \
{"M5scl1"}, \
{"M5scl2"}, \
{"M5scl3"}, \
{"M5scl4"}, \
{"M5Aen"}, \
{"M5Asrc"}, \
{"M5Aatt"}, \
{"M5Acrv"}, \
{"M5rngA"}, \
{"M5rngB"}, \
{"M5rngXA"}, \
{"M5rngXB"}, \
{"M6en"}, \
{"M6src"}, \
{"M6dest1"}, \
{"M6dest2"}, \
{"M6dest3"}, \
{"M6dest4"}, \
{"M6curv"}, \
{"M6scl1"}, \
{"M6scl2"}, \
{"M6scl3"}, \
{"M6scl4"}, \
{"M6Aen"}, \
{"M6Asrc"}, \
{"M6Aatt"}, \
{"M6Acrv"}, \
{"M6rngA"}, \
{"M6rngB"}, \
{"M6rngXA"}, \
{"M6rngXB"}, \
{"M7en"}, \
{"M7src"}, \
{"M7dest1"}, \
{"M7dest2"}, \
{"M7dest3"}, \
{"M7dest4"}, \
{"M7curv"}, \
{"M7scl1"}, \
{"M7scl2"}, \
{"M7scl3"}, \
{"M7scl4"}, \
{"M7Aen"}, \
{"M7Asrc"}, \
{"M7Aatt"}, \
{"M7Acrv"}, \
{"M7rngA"}, \
{"M7rngB"}, \
{"M7rngXA"}, \
{"M7rngXB"}, \
{"M8en"}, \
{"M8src"}, \
{"M8dest1"}, \
{"M8dest2"}, \
{"M8dest3"}, \
{"M8dest4"}, \
{"M8curv"}, \
{"M8scl1"}, \
{"M8scl2"}, \
{"M8scl3"}, \
{"M8scl4"}, \
{"M8Aen"}, \
{"M8Asrc"}, \
{"M8Aatt"}, \
{"M8Acrv"}, \
{"M8rngA"}, \
{"M8rngB"}, \
{"M8rngXA"}, \
{"M8rngXB"}, \
{"M9en"}, \
{"M9src"}, \
{"M9dest1"}, \
{"M9dest2"}, \
{"M9dest3"}, \
{"M9dest4"}, \
{"M9curv"}, \
{"M9scl1"}, \
{"M9scl2"}, \
{"M9scl3"}, \
{"M9scl4"}, \
{"M9Aen"}, \
{"M9Asrc"}, \
{"M9Aatt"}, \
{"M9Acrv"}, \
{"M9rngA"}, \
{"M9rngB"}, \
{"M9rngXA"}, \
{"M9rngXB"}, \
{"M10en"}, \
{"M10src"}, \
{"M10dst1"}, \
{"M10dst2"}, \
{"M10dst3"}, \
{"M10dst4"}, \
{"M10curv"}, \
{"M10scl1"}, \
{"M10scl2"}, \
{"M10scl3"}, \
{"M10scl4"}, \
{"M10Aen"}, \
{"M10Asrc"}, \
{"M10Aatt"}, \
{"M10Acrv"}, \
{"M10rgA"}, \
{"M10rgB"}, \
{"M10rgXA"}, \
{"M10rgXB"}, \
{"M11en"}, \
{"M11src"}, \
{"M11dst1"}, \
{"M11dst2"}, \
{"M11dst3"}, \
{"M11dst4"}, \
{"M11curv"}, \
{"M11scl1"}, \
{"M11scl2"}, \
{"M11scl3"}, \
{"M11scl4"}, \
{"M11Aen"}, \
{"M11Asrc"}, \
{"M11Aatt"}, \
{"M11Acrv"}, \
{"M11rgA"}, \
{"M11rgB"}, \
{"M11rgXA"}, \
{"M11rgXB"}, \
{"M12en"}, \
{"M12src"}, \
{"M12dst1"}, \
{"M12dst2"}, \
{"M12dst3"}, \
{"M12dst4"}, \
{"M12curv"}, \
{"M12scl1"}, \
{"M12scl2"}, \
{"M12scl3"}, \
{"M12scl4"}, \
{"M12Aen"}, \
{"M12Asrc"}, \
{"M12Aatt"}, \
{"M12Acrv"}, \
{"M12rgA"}, \
{"M12rgB"}, \
{"M12rgXA"}, \
{"M12rgXB"}, \
{"M13en"}, \
{"M13src"}, \
{"M13dst1"}, \
{"M13dst2"}, \
{"M13dst3"}, \
{"M13dst4"}, \
{"M13curv"}, \
{"M13scl1"}, \
{"M13scl2"}, \
{"M13scl3"}, \
{"M13scl4"}, \
{"M13Aen"}, \
{"M13Asrc"}, \
{"M13Aatt"}, \
{"M13Acrv"}, \
{"M13rgA"}, \
{"M13rgB"}, \
{"M13rgXA"}, \
{"M13rgXB"}, \
{"M14en"}, \
{"M14src"}, \
{"M14dst1"}, \
{"M14dst2"}, \
{"M14dst3"}, \
{"M14dst4"}, \
{"M14curv"}, \
{"M14scl1"}, \
{"M14scl2"}, \
{"M14scl3"}, \
{"M14scl4"}, \
{"M14Aen"}, \
{"M14Asrc"}, \
{"M14Aatt"}, \
{"M14Acrv"}, \
{"M14rgA"}, \
{"M14rgB"}, \
{"M14rgXA"}, \
{"M14rgXB"}, \
{"M15en"}, \
{"M15src"}, \
{"M15dst1"}, \
{"M15dst2"}, \
{"M15dst3"}, \
{"M15dst4"}, \
{"M15curv"}, \
{"M15scl1"}, \
{"M15scl2"}, \
{"M15scl3"}, \
{"M15scl4"}, \
{"M15Aen"}, \
{"M15Asrc"}, \
{"M15Aatt"}, \
{"M15Acrv"}, \
{"M15rgA"}, \
{"M15rgB"}, \
{"M15rgXA"}, \
{"M15rgXB"}, \
{"M16en"}, \
{"M16src"}, \
{"M16dst1"}, \
{"M16dst2"}, \
{"M16dst3"}, \
{"M16dst4"}, \
{"M16curv"}, \
{"M16scl1"}, \
{"M16scl2"}, \
{"M16scl3"}, \
{"M16scl4"}, \
{"M16Aen"}, \
{"M16Asrc"}, \
{"M16Aatt"}, \
{"M16Acrv"}, \
{"M16rgA"}, \
{"M16rgB"}, \
{"M16rgXA"}, \
{"M16rgXB"}, \
{"M17en"}, \
{"M17src"}, \
{"M17dst1"}, \
{"M17dst2"}, \
{"M17dst3"}, \
{"M17dst4"}, \
{"M17curv"}, \
{"M17scl1"}, \
{"M17scl2"}, \
{"M17scl3"}, \
{"M17scl4"}, \
{"M17Aen"}, \
{"M17Asrc"}, \
{"M17Aatt"}, \
{"M17Acrv"}, \
{"M17rgA"}, \
{"M17rgB"}, \
{"M17rgXA"}, \
{"M17rgXB"}, \
{"M18en"}, \
{"M18src"}, \
{"M18dst1"}, \
{"M18dst2"}, \
{"M18dst3"}, \
{"M18dst4"}, \
{"M18curv"}, \
{"M18scl1"}, \
{"M18scl2"}, \
{"M18scl3"}, \
{"M18scl4"}, \
{"M18Aen"}, \
{"M18Asrc"}, \
{"M18Aatt"}, \
{"M18Acrv"}, \
{"M18rgA"}, \
{"M18rgB"}, \
{"M18rgXA"}, \
{"M18rgXB"}, \
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
{"S1Dly"}, \
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
{"S1Emode"}, \
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
{"S2Dly"}, \
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
{"S2Emode"}, \
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
{"S3Dly"}, \
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
{"S3Emode"}, \
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
{"S4Dly"}, \
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
{"S4Emode"}, \
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

            PortamentoTime,
            PortamentoCurve,
            PitchBendRange,

            MaxVoices,

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
            Delay,
            AmpEnvDelayTime,
            Count = AmpEnvDelayTime,
        };

        enum class ModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Enabled,
            Source,
            Destination1,
            Destination2,
            Destination3,
            Destination4,
            Curve,
            Scale1,
            Scale2,
            Scale3,
            Scale4,
            AuxEnabled,
            AuxSource,
            AuxAttenuation,
            AuxCurve,

            SrcRangeMin,
            SrcRangeMax,
            AuxRangeMin,
            AuxRangeMax,

            Count,
        };
        enum class LFOParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
        {
            Waveform,
            Waveshape,
            Restart,
            PhaseOffset,
            FrequencyBasis,
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
            Mode,
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


        // FILTER AUX INFO  ------------------------------------------------------------
        enum class FilterParamIndexOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets & FilterAuxModIndexOffsets
        {
            Enabled,
            FilterType, // filter type
            Q, // param2: filter Q
            Freq, // filter freq
            FreqKT, // filter KT
            Count,
        };

        enum class FilterAuxModDestOffsets : uint8_t // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets
        {
            Q, // filter Q
            Freq, // filter freq
            Count,
        };

        // ------------------------------------------------------------
        enum class EnvelopeMode : uint8_t
        {
            Sustain, // uses all features of the envelope.
            OneShot, // ignores note offs, decays to 0.
            Count,
        };



    } // namespace M7


} // namespace WaveSabreCore








