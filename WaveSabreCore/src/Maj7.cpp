#include <WaveSabreCore/Maj7.hpp>
namespace WaveSabreCore {
    namespace M7 {
        static_assert((int)M7::MainParamIndices::Count == 32, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultMasterParams[32] = {
          16422, // Master = 0.50118720531463623047
          8, // PolyMon = 0.0002470355830155313015
          1365, // Unisono = 0.041666667908430099487
          0, // OscDet = 0
          0, // UniDet = 0
          0, // OscSpr = 0
          0, // UniSpr = 0
          16384, // FMBrigh = 0.5
          0, // _xrt___ = 0
          32767, // XWidth = 1
          9830, // PortTm = 0.30000001192092895508
          16384, // PortCv = 0.5
          17721, // PBRng = 0.54081630706787109375
          12032, // MaxVox = 0.3671875
          31644, // Macro1 = 0.96571481227874755859
          0, // Macro2 = 0
          0, // Macro3 = 0
          0, // Macro4 = 0
          0, // Macro5 = 0
          0, // Macro6 = 0
          0, // Macro7 = 0
          0, // FM2to1 = 0
          0, // FM3to1 = 0
          0, // FM4to1 = 0
          0, // FM1to2 = 0
          0, // FM3to2 = 0
          0, // FM4to2 = 0
          0, // FM1to3 = 0
          0, // FM2to3 = 0
          0, // FM4to3 = 0
          0, // FM1to4 = 0
          0, // FM2to4 = 0
        };
        static_assert((int)M7::SamplerParamIndexOffsets::Count == 21, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultSamplerParams[21] = {
          0, // S1En = 0
          32767, // S1Vol = 1
          128, // S1KRmin = 0.00390625
          32640, // S1KRmax = 0.99609375
          15488, // S1base = 0.47265625
          32767, // S1LTrig = 1
          0, // S1Rev = 0
          24, // S1src = 0.00074110669083893299103
          32, // S1gmidx = 0.0010060361819341778755
          0, // S1strt = 0
          24, // S1LMode = 0.00074110669083893299103
          8, // S1LSrc = 0.0002470355830155313015
          0, // S1Lbeg = 0
          32767, // S1Llen = 1
          16384, // S1TunS = 0.5
          16384, // S1TunF = 0.5
          9830, // S1Frq = 0.30000001192092895508
          32767, // S1FrqKT = 1
          24, // S1Intrp = 0.00074110669083893299103
          32767, // S1RelX = 1
          16384, // S1AxMix = 0.5
        };
        static_assert((int)M7::ModParamIndexOffsets::Count == 17, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultModSpecParams[17] = {
          0, // M1en = 0
          8, // M1src = 0.0002470355830155313015
          8, // M1dest1 = 0.0002470355830155313015
          8, // M1dest2 = 0.0002470355830155313015
          8, // M1dest3 = 0.0002470355830155313015
          8, // M1dest4 = 0.0002470355830155313015
          16384, // M1curv = 0.5
          28672, // M1scl1 = 0.875
          28672, // M1scl2 = 0.875
          28672, // M1scl3 = 0.875
          28672, // M1scl4 = 0.875
          0, // M1Aen = 0
          8, // M1Asrc = 0.0002470355830155313015
          32767, // M1Aatt = 1
          16384, // M1Acrv = 0.5
          8, // M1map = 0.0002470355830155313015
          8, // M1Amap = 0.0002470355830155313015
        };
        static_assert((int)M7::LFOParamIndexOffsets::Count == 6, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultLFOParams[6] = {
          121, // LFO1wav = 0.003705533687025308609
          16384, // LFO1shp = 0.5
          0, // LFO1rst = 0
          16384, // LFO1ph = 0.5
          19660, // LFO1fr = 0.60000002384185791016
          16384, // LFO1lp = 0.5
        };
        static_assert((int)M7::EnvParamIndexOffsets::Count == 11, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultEnvelopeParams[11] = {
          0, // AE1dlt = 0
          1638, // AE1att = 0.050000000745058059692
          24576, // AE1atc = 0.75
          0, // AE1ht = 0
          16384, // AE1dt = 0.5
          8192, // AE1dc = 0.25
          13107, // AE1sl = 0.40000000596046447754
          6553, // AE1rt = 0.20000000298023223877
          8192, // AE1tc = 0.25
          32767, // AE1rst = 1
          0, // AE1mode = 0
        };
        static_assert((int)M7::OscParamIndexOffsets::Count == 18, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultOscillatorParams[18] = {
          0, // O1En = 0
          32767, // O1Vol = 1
          128, // O1KRmin = 0.00390625
          32640, // O1KRmax = 0.99609375
          56, // O1Wave = 0.001729249022901058197
          16384, // O1Shp = 0.5
          0, // O1PRst = 0
          16384, // O1Poff = 0.5
          0, // O1Scen = 0
          9830, // O1ScFq = 0.30000001192092895508
          32767, // O1ScKt = 1
          9830, // O1Fq = 0.30000001192092895508
          32767, // O1FqKt = 1
          16384, // O1Semi = 0.5
          16384, // O1Fine = 0.5
          512, // O1Mul = 0.015625
          0, // O1FMFb = 0
          16384, // O1Xmix = 0.5
        };
        static_assert((int)M7::FilterParamIndexOffsets::Count == 8, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultFilterParams[8] = {
          0, // F1En = 0
          0, // _x1lnk_ = 0
          0, // _x1tp_ = 0
          56, // F1Type = 0.001729249022901058197
          6553, // F1Q = 0.20000000298023223877
          0, // _x1p3_ = 0
          9830, // F1Freq = 0.30000001192092895508
          32767, // F1FKT = 1
        };
    } // namespace M7
} // namespace WaveSabreCore
