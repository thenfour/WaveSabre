#include <WaveSabreCore/Maj7.hpp>
namespace WaveSabreCore {
    namespace M7 {
        static_assert((int)M7::MainParamIndices::Count == 29, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultMasterParams[29] = {
          16422, // Master = 0.50115966796875
          8, // PolyMon = 0.000244140625
          1365, // Unisono = 0.041656494140625
          0, // UniDet = 0
          0, // UniSpr = 0
          16384, // FMBrigh = 0.5
          5996, // PortTm = 0.1829833984375
          17721, // PBRng = 0.540802001953125
          12032, // MaxVox = 0.3671875
          16384, // pan
          0, // Macro1 = 0
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
          0, // FM3to4 = 0
        };
        static_assert((int)M7::SamplerParamIndexOffsets::Count == 21, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultSamplerParams[21] = {
          0, // S1En = 0
          32767, // S1Vol = 0.999969482421875
          128, // S1KRmin = 0.00390625
          32640, // S1KRmax = 0.99609375
          15488, // S1base = 0.47265625
          32767, // S1LTrig = 0.999969482421875
          0, // S1Rev = 0
          24, // S1src = 0.000732421875
          32, // S1gmidx = 0.0009765625
          0, // S1strt = 0
          24, // S1LMode = 0.000732421875
          8, // S1LSrc = 0.000244140625
          0, // S1Lbeg = 0
          32767, // S1Llen = 0.999969482421875
          16384, // S1TunS = 0.5
          16384, // S1TunF = 0.5
          9830, // S1Frq = 0.29998779296875
          32767, // S1FrqKT = 0.999969482421875
          24, // S1Intrp = 0.000732421875
          32767, // S1RelX = 0.999969482421875
          0, // S1Dly = 0
        };
        static_assert((int)M7::ModParamIndexOffsets::Count == 19, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultModSpecParams[19] = {
          0, // M1en = 0
          8, // M1src = 0.000244140625
          8, // M1dest1 = 0.000244140625
          8, // M1dest2 = 0.000244140625
          8, // M1dest3 = 0.000244140625
          8, // M1dest4 = 0.000244140625
          16384, // M1curv = 0.5
          28672, // M1scl1 = 0.875
          28672, // M1scl2 = 0.875
          28672, // M1scl3 = 0.875
          28672, // M1scl4 = 0.875
          0, // M1Aen = 0
          8, // M1Asrc = 0.000244140625
          32767, // M1Aatt = 0.999969482421875
          16384, // M1Acrv = 0.5
          10922, // M1rngA = 0.33331298828125
          21845, // M1rngB = 0.666656494140625
          16384, // M1rngXA = 0.5
          21845, // M1rngXB = 0.666656494140625
        };
        static_assert((int)M7::LFOParamIndexOffsets::Count == 7, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultLFOParams[7] = {
          105, // LFO1wav = 0.0032114624045789241791
          16384, // LFO1shp = 0.5
          0, // LFO1rst = 0
          16384, // LFO1ph = 0.5
          8, // LFO1bas = 0.000244140625
          19660, // LFO1fr = 0.5999755859375
          16384, // LFO1lp = 0.5
        };
        static_assert((int)M7::EnvParamIndexOffsets::Count == 11, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultEnvelopeParams[11] = {
          0, // AE1dlt = 0
          1638, // AE1att = 0.04998779296875
          24576, // AE1atc = 0.75
          0, // AE1ht = 0
          16384, // AE1dt = 0.5
          8192, // AE1dc = 0.25
          13107, // AE1sl = 0.399993896484375
          6553, // AE1rt = 0.199981689453125
          8192, // AE1tc = 0.25
          32767, // AE1rst = 0.999969482421875
          0, // AE1mode = 0
        };
        static_assert((int)M7::OscParamIndexOffsets::Count == 17, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultOscillatorParams[17] = {
          0, // O1En = 0
          32767, // O1Vol = 0.999969482421875
          128, // O1KRmin = 0.00390625
          32640, // O1KRmax = 0.99609375
          56, // O1Wave = 0.001708984375
          16384, // O1Shp = 0.5
          0, // O1PRst = 0
          16384, // O1Poff = 0.5
          0, // O1Scen = 0
          9830, // O1ScFq = 0.29998779296875
          32767, // O1ScKt = 0.999969482421875
          9830, // O1Fq = 0.29998779296875
          32767, // O1FqKt = 0.999969482421875
          16384, // O1Semi = 0.5
          16384, // O1Fine = 0.5
          512, // O1Mul = 0.015625
          0, // O1FMFb = 0
        };
        static_assert((int)M7::FilterParamIndexOffsets::Count == 5, "param count probably changed and this needs to be regenerated.");
        const int16_t gDefaultFilterParams[5] = {
          0, // F1En = 0
          153, // F1Type = 0.004669189453125
          6553, // F1Q = 0.199981689453125
          9830, // F1Freq = 0.29998779296875
          32767, // F1FKT = 0.999969482421875
        };
    } // namespace M7
} // namespace WaveSabreCore
