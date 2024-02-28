
#pragma once

#include <Windows.h>
#include <memory>
#include <algorithm>

namespace WaveSabreCore
{
    namespace M7
    {
#define INLINE inline
#ifdef MIN_SIZE_REL
#define FORCE_INLINE inline
#else
        // for non-size-optimized builds (bloaty), force inline some things when profiling suggests its performant.
#define FORCE_INLINE __forceinline
#endif
        namespace math
        {
            extern void InitLUTs();

            using real_t = float;

            static constexpr float gPI = 3.14159265358979323846264338327950288f;
            static constexpr float gPITimes2 = gPI * 2;
            static constexpr float gPIHalf = gPI * 0.5f;
            static constexpr float gSqrt2 = 1.41421356237f;
            static constexpr float gSqrt2Recip = 0.70710678118f;
            static constexpr float FloatEpsilon = 0.000001f;

            INLINE real_t floor(real_t x) {
                return (real_t)::floor((double)x);
            }
            INLINE double floord(double x) {
                return ::floor(x);
            }
            INLINE float rand01() {
                return float(::rand()) / RAND_MAX;
            }
            INLINE float randN11() {
                return rand01() * 2 - 1;
            }
            INLINE real_t sqrt(real_t x) {
                return (float)::sqrt((double)x);
            }
            INLINE constexpr real_t clamp(real_t x, real_t low, real_t hi)
            {
                if (x <= low)
                    return low;
                if (x >= hi)
                    return hi;
                return x;
            }
            INLINE constexpr real_t clamp01(real_t x)
            {
                return clamp(x, 0, 1);
            }
            INLINE constexpr real_t clampN11(real_t x)
            {
                return clamp(x, -1, 1);
            }

            template <typename T>
            INLINE static T ClampI(T x, T minInclusive, T maxInclusive)
            {
                if (x <= minInclusive)
                    return minInclusive;
                if (x >= maxInclusive)
                    return maxInclusive;
                return x;
            }


#ifdef MIN_SIZE_REL
            float lerp(float a, float b, float t);
            float lerp_rev(float v_min, float v_max, float v_val);
            float fract(float x);
            double fract(double x);
#else
            INLINE float lerp(float a, float b, float t)
            {
                return a * (1.0f - t) + b * t;
            }
            INLINE float lerp_rev(float v_min, float v_max, float v_val) // inverse of lerp; returns 0-1 where t lies between a and b.
            {
                return (v_val - v_min) / (v_max - v_min);
            }
            INLINE float fract(float x) {
                return x - math::floor(x);
            }
            INLINE double fract(double x) {
                return x - math::floord(x);
            }
#endif

            INLINE float map(float inp, float inp_min, float inp_max, float out_min, float out_max)
            {
                float t = lerp_rev(inp_min, inp_max, inp);
                return lerp(out_min, out_max, t);
            }
            INLINE float map_clamped(float inp, float inp_min, float inp_max, float out_min, float out_max)
            {
                float t = lerp_rev(inp_min, inp_max, inp);
                return lerp(out_min, out_max, clamp01(t));
            }

            void* GetCrtProc(const char* const imp);

            struct CrtFns {
                CrtFns();
                double(__cdecl* crt_sin)(double);
                double(__cdecl* crt_cos)(double);
                double(__cdecl* crt_tan)(double);
                double(__cdecl* crt_floor)(double);
                double(__cdecl* crt_log)(double);
                double(__cdecl* crt_pow)(double, double);
                double(__cdecl* crt_exp)(double);
            };

            extern CrtFns* gCrtFns;

            INLINE double CrtSin(double x) {
                return gCrtFns->crt_sin(x);
            }

            INLINE double CrtCos(double x) {
                return gCrtFns->crt_cos(x);
            }

            INLINE double CrtTan(double x) {
                return gCrtFns->crt_tan(x);
            }

            INLINE double CrtFloor(double x) {
                return gCrtFns->crt_floor(x);
            }

            INLINE double CrtLog(double x) {
                return gCrtFns->crt_log(x);
            }

            INLINE double CrtPow(double x, double y) {
                return gCrtFns->crt_pow(x, y);
            }

            // MSVC has weird rules about implicitly calling CRT functions to cast between floats & integers. this encapsulates the chaos.
            INLINE long DoubleToLong(double x) {
                return (long)x;
            }

            // MSVC has weird rules about implicitly calling CRT functions to cast between floats & integers. this encapsulates the chaos.
            INLINE long FloatToLong(float x) {
                return (long)x;
            }

            INLINE real_t pow(real_t x, real_t y) {
                return (float)CrtPow((double)x, (double)y);
            }
            INLINE real_t pow2(real_t y) {
                return (float)CrtPow(2, (double)y);
            }

            INLINE float log2(float x) {
                static constexpr float log2_e = 1.44269504089f; // 1/log(2)
                return (float)CrtLog((double)x) * log2_e;
            }
            INLINE float log10(float x) {
                static constexpr float log10_e = 0.4342944819f;// 1.0f / LOG(10.0f);
                return (float)CrtLog((double)x) * log10_e;
            }
            INLINE real_t tan(real_t x) {
                return (float)CrtTan((double)x);// fastmath::fastertanfull(x); // less fast to call lib function, but smaller code.
            }
            INLINE float expf(float x) {
                return (float)gCrtFns->crt_exp((double)x);
            }


            static constexpr size_t gLutSize1D = 4096;
            static constexpr size_t gLutSize2D = 512;



            // LUTs are such hot paths we want to inline for non-size-optimized builds
#ifdef MIN_SIZE_REL


            // lookup table for 0-1 input values, linear interpolation upon lookup.
            // non-periodic.
            struct LUT01
            {
                const size_t mNSamples;
                float* mpTable;

                LUT01(size_t nSamples, float (*fn)(float));
                //#ifdef MIN_SIZE_REL
                //#else
                //                virtual ~LUT01();
                //#endif // MIN_SIZE_REL
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
                //#ifdef MIN_SIZE_REL
                //#else
                //                virtual ~LUT2D();
                //#endif // MIN_SIZE_REL

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
                LUT01 gSqrt01LUT;

                CurveLUT gCurveLUT;
                Pow2_N16_16_LUT gPow2_N16_16_LUT;
                // pow10 could be useful too.

                LUTs();
            };

#else

            template<size_t N>
            INLINE float LookupLUT1D(const float (&mpTable)[N], float x)
            {
                if (x <= 0) return mpTable[0];
                if (x >= 1) return mpTable[gLutSize1D - 1];
                float index = x * (gLutSize1D - 1);
                int lower_index = static_cast<int>(index);
                int upper_index = lower_index + 1;
                float t = index - lower_index;
                return (1 - t) * mpTable[lower_index] + t * mpTable[upper_index];
            }

            struct SinLUT
            {
                float mpTable[gLutSize1D];

                INLINE SinLUT()
                {
                    for (size_t i = 0; i < gLutSize1D; ++i) {
                        float x = float(i) / (gLutSize1D - 1);
                        mpTable[i] = (float)math::CrtSin((double)(x * gPITimes2));
                    }
                }

                INLINE float Invoke(float x) const
                {
                    static constexpr float gPeriodCorrection = 1 / gPITimes2;
                    float scaledX = x * gPeriodCorrection;
                    return LookupLUT1D(mpTable, fract(scaledX));
                }
            };

            struct CosLUT
            {
                float mpTable[gLutSize1D];

                INLINE CosLUT()
                {
                    for (size_t i = 0; i < gLutSize1D; ++i) {
                        float x = float(i) / (gLutSize1D - 1);
                        mpTable[i] = (float)math::CrtCos((double)(x * gPITimes2));
                    }
                }

                INLINE float Invoke(float x) const
                {
                    static constexpr float gPeriodCorrection = 1 / gPITimes2;
                    float scaledX = x * gPeriodCorrection;
                    return LookupLUT1D(mpTable, fract(scaledX));
                }
            };

            struct Sqrt01LUT
            {
                float mpTable[gLutSize1D];

                INLINE Sqrt01LUT()
                {
                    for (size_t i = 0; i < gLutSize1D; ++i) {
                        float x = float(i) / (gLutSize1D - 1);
                        mpTable[i] = (float)math::CrtPow((double)x, 0.5);
                    }
                }

                INLINE float Invoke(float x) const
                {
                    return LookupLUT1D(mpTable, x);
                }
            };

            // tanh approaches -1 before -PI, and +1 after +PI. so squish the range and do a 0,1 LUT mapping from -PI,PI
            struct TanHLUT
            {
                float mpTable[gLutSize1D];

                INLINE TanHLUT()
                {
                    for (size_t i = 0; i < gLutSize1D; ++i) {
                        float x = float(i) / (gLutSize1D - 1);
                        mpTable[i] = (float)::tanh(((double)x - .5) * gPITimes2);
                    }
                }
                INLINE float Invoke(float x) const
                {
                    static constexpr float gOneOver2pi = 1.0f / gPITimes2;
                    x *= gOneOver2pi;
                    x += 0.5f;
                    return LookupLUT1D(mpTable, x);
                }
            };

            // pow(2,n) is a quite hot path, used by MidiNoteToFrequency(), as well as all other frequency calculations.
            // the range of `n` is [-15,+15] but depends on frequency param scale. so let's extend a bit and make a huge lut.
            struct Pow2_N16_16_LUT
            {
                float mpTable[gLutSize1D];

                INLINE Pow2_N16_16_LUT()
                {
                    for (size_t i = 0; i < gLutSize1D; ++i) {
                        float x = float(i) / (gLutSize1D - 1);
                        mpTable[i] = (float)math::CrtPow(2.0, ((double)x - .5) * 32);
                    }
                }
                INLINE float Invoke(float x) const
                {
                    static constexpr float gScaleCorrection = 1.0f / 32;
                    x *= gScaleCorrection;
                    x += 0.5f;
                    return LookupLUT1D(mpTable, x);
                }
            };

            INLINE float LookupLUT2D(const float(&mpTable)[gLutSize2D * gLutSize2D], float x, float y)
            {
                if (x <= 0) x = 0;
                if (y <= 0) y = 0;
                if (x >= 1) x = 0.9999f; // this ensures it will not OOB
                if (y >= 1) y = 0.9999f;

                float indexX = x * (gLutSize2D - 1);
                float indexY = y * (gLutSize2D - 1);

                int lowerIndexX = static_cast<int>(indexX);
                int lowerIndexY = static_cast<int>(indexY);
                int upperIndexX = lowerIndexX + 1;
                int upperIndexY = lowerIndexY + 1;

                float tx = indexX - lowerIndexX;
                float ty = indexY - lowerIndexY;

                float f00 = mpTable[lowerIndexY * gLutSize2D + lowerIndexX];
                float f10 = mpTable[lowerIndexY * gLutSize2D + upperIndexX];
                float f0 = lerp(f00, f10, tx);

                float f01 = mpTable[upperIndexY * gLutSize2D + lowerIndexX];
                float f11 = mpTable[upperIndexY * gLutSize2D + upperIndexX];
                float f1 = lerp(f01, f11, tx);

                return lerp(f0, f1, ty);
            }

            struct CurveLUT// : public LUT2D
            {
                float mpTable[gLutSize2D * gLutSize2D];

                // incoming values should support -1,1 and output -1,1
                INLINE CurveLUT()
                {
                    // Populate table.
                    for (size_t y = 0; y < gLutSize2D; ++y) {
                        for (size_t x = 0; x < gLutSize2D; ++x) {
                            float x01 = float(x) / (gLutSize2D - 1);
                            float y01 = float(y) / (gLutSize2D - 1);
                            mpTable[y * gLutSize2D + x] = modCurve_xN11_kN11_RT(x01 * 2 - 1, y01 * 2 - 1);
                        }
                    }
                }

                // valid for 0<k<1 and 0<x<1
                INLINE static real_t modCurve_x01_k01_RT(real_t x, real_t k)
                {
                    real_t ret = 1 - math::pow(x, k);
                    return math::pow(ret, 1 / k);
                }

                // extends range to support -1<x<0 and -1<k<0
                // outputs -1 to 1
                INLINE static real_t modCurve_xN11_kN11_RT(real_t x, real_t k)
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

                // user passes in n11 values; need to map N11 to 01
                INLINE float Invoke(float xN11, float yN11) const
                {
                    return LookupLUT2D(mpTable, xN11 * .5f + .5f, yN11 * .5f + .5f);
                }
            };

            struct LUTs
            {
                SinLUT gSinLUT;
                CosLUT gCosLUT;
                TanHLUT gTanhLUT;
                Sqrt01LUT gSqrt01LUT;
                Pow2_N16_16_LUT gPow2_N16_16_LUT;

                CurveLUT gCurveLUT;
                // pow10 could be useful too.
            };


#endif // MIN_SIZE_REL

            extern LUTs* gLuts;

            INLINE float pow2_N16_16(float n)
            {
                return gLuts->gPow2_N16_16_LUT.Invoke(n);
            }
            INLINE real_t sin(real_t x)
            {
                return gLuts->gSinLUT.Invoke(x);
            }
            INLINE real_t cos(real_t x)
            {
                return gLuts->gCosLUT.Invoke(x);
            }
            // optimized LUT works for 0<=x<=1
            INLINE real_t sqrt01(real_t x)
            {
                return gLuts->gSqrt01LUT.Invoke(x);
            }
            INLINE real_t tanh(real_t x)
            {
                return gLuts->gTanhLUT.Invoke(x);
            }
            INLINE real_t modCurve_xN11_kN11(real_t x, real_t k)
            {
                return gLuts->gCurveLUT.Invoke(x, k);
            }

        } // namespace math


    } // namespace M7


} // namespace WaveSabreCore








