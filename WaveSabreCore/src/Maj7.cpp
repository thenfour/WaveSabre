#include <WaveSabreCore/Maj7.hpp>
namespace WaveSabreCore {
    namespace M7 {
        static_assert((int)M7::MainParamIndices::Count == 33, "param count probably changed and this needs to be regenerated.");
        const float gDefaultMasterParams[33] = {
          0.50118720531463623047, // Master
          0.0002470355830155313015, // PolyMon
          0.041666667908430099487, // Unisono
          0, // OscDet
          0, // UniDet
          0, // OscSpr
          0, // UniSpr
          0.5, // FMBrigh
          0.0002470355830155313015, // XRout
          1, // XWidth
          0.30000001192092895508, // PortTm
          0.5, // PortCv
          0.54081630706787109375, // PBRng
          0.3671875, // MaxVox
          0, // Macro1
          0, // Macro2
          0, // Macro3
          0, // Macro4
          0, // Macro5
          0, // Macro6
          0, // Macro7
          0, // FM2to1
          0, // FM3to1
          0, // FM4to1
          0, // FM1to2
          0, // FM3to2
          0, // FM4to2
          0, // FM1to3
          0, // FM2to3
          0, // FM4to3
          0, // FM1to4
          0, // FM2to4
          0, // FM3to4
        };
        static_assert((int)M7::SamplerParamIndexOffsets::Count == 21, "param count probably changed and this needs to be regenerated.");
        const float gDefaultSamplerParams[21] = {
          0, // S1En
          1, // S1Vol
          0.00390625, // S1KRmin
          0.99609375, // S1KRmax
          0.47265625, // S1base
          1, // S1LTrig
          0, // S1Rev
          0.00074110669083893299103, // S1src
          0.0010060361819341778755, // S1gmidx
          0, // S1strt
          0.00074110669083893299103, // S1LMode
          0.0002470355830155313015, // S1LSrc
          0, // S1Lbeg
          1, // S1Llen
          0.5, // S1TunS
          0.5, // S1TunF
          0.30000001192092895508, // S1Frq
          1, // S1FrqKT
          0.00074110669083893299103, // S1Intrp
          1, // S1RelX
          0.5, // S1AxMix
        };
        static_assert((int)M7::ModParamIndexOffsets::Count == 17, "param count probably changed and this needs to be regenerated.");
        const float gDefaultModSpecParams[17] = {
          0, // M1en
          0.0002470355830155313015, // M1src
          0.0002470355830155313015, // M1dest1
          0.0002470355830155313015, // M1dest2
          0.0002470355830155313015, // M1dest3
          0.0002470355830155313015, // M1dest4
          0.5, // M1curv
          0.875, // M1scl1
          0.875, // M1scl2
          0.875, // M1scl3
          0.875, // M1scl4
          0, // M1Aen
          0.0002470355830155313015, // M1Asrc
          1, // M1Aatt
          0.5, // M1Acrv
          0.0002470355830155313015, // M1map
          0.0002470355830155313015, // M1Amap
        };
        static_assert((int)M7::LFOParamIndexOffsets::Count == 6, "param count probably changed and this needs to be regenerated.");
        const float gDefaultLFOParams[6] = {
          0.003705533687025308609, // LFO1wav
          0.5, // LFO1shp
          0, // LFO1rst
          0.5, // LFO1ph
          0.60000002384185791016, // LFO1fr
          0.5, // LFO1lp
        };
        static_assert((int)M7::EnvParamIndexOffsets::Count == 11, "param count probably changed and this needs to be regenerated.");
        const float gDefaultEnvelopeParams[11] = {
          0, // AE1dlt
          0.050000000745058059692, // AE1att
          0.75, // AE1atc
          0, // AE1ht
          0.5, // AE1dt
          0.25, // AE1dc
          0.40000000596046447754, // AE1sl
          0.20000000298023223877, // AE1rt
          0.25, // AE1tc
          1, // AE1rst
          0, // AE1mode
        };
        static_assert((int)M7::OscParamIndexOffsets::Count == 18, "param count probably changed and this needs to be regenerated.");
        const float gDefaultOscillatorParams[18] = {
          0, // O1En
          1, // O1Vol
          0.00390625, // O1KRmin
          0.99609375, // O1KRmax
          0.001729249022901058197, // O1Wave
          0.5, // O1Shp
          0, // O1PRst
          0.5, // O1Poff
          0, // O1Scen
          0.30000001192092895508, // O1ScFq
          1, // O1ScKt
          0.30000001192092895508, // O1Fq
          1, // O1FqKt
          0.5, // O1Semi
          0.5, // O1Fine
          0.015625, // O1Mul
          0, // O1FMFb
          0.5, // O1Xmix
        };
        static_assert((int)M7::AuxParamIndexOffsets::Count == 8, "param count probably changed and this needs to be regenerated.");
        const float gDefaultAuxParams[8] = {
          0, // X1En
          0.0002470355830155313015, // X1Link
          0.0002470355830155313015, // X1Type
          0, // X1P1
          0, // X1P2
          0, // X1P3
          0, // X1P4
          0, // X1P5
        };
    } // namespace M7
} // namespace WaveSabreCore
