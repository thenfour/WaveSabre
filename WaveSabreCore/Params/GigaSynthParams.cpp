#include "../GigaSynth/GigaParams.hpp"
namespace WaveSabreCore {
  namespace M7 {
    static_assert((int)M7::MainParamIndices::Count == 26, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultMasterParams[26] = {
      16422, // Master = 0.50118726491928100586
      0, // Pan = 0
      0, // PolyMon = 0
      3, // Unisono = 0.0001220703125
      0, // UniDet = 0
      0, // UniSpr = 0
      16383, // FMBrigh = 0.5
      9830, // PortTm = 0.30000001192092895508
      7, // PBRng = 0.000244140625
      95, // MaxVox = 0.0029296875
      0, // Macro1 = 0
      0, // Macro2 = 0
      0, // Macro3 = 0
      0, // Macro4 = 0
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
    static_assert((int)M7::SamplerParamIndexOffsets::Count == 20, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultSamplerParams[20] = {
      0, // S1En = 0
      32767, // S1Vol = 1
      0, // S1Pan = 0
      0, // S1KRmin = 0
      507, // S1KRmax = 0.0155029296875
      239, // S1base = 0.00732421875
      32767, // S1LTrig = 1
      0, // S1Rev = 0
      0, // S1gmidx = 0
      0, // S1strt = 0
      3, // S1LMode = 0.0001220703125
      0, // S1LSrc = 0
      0, // S1Lbeg = 0
      32767, // S1Llen = 1
      0, // S1TunS = 0
      0, // S1TunF = 0
      9830, // S1Frq = 0.30000001192092895508
      32767, // S1FrqKT = 1
      32767, // S1RelX = 1
      0, // S1Dly = 0
    };
    static_assert((int)M7::ModParamIndexOffsets::Count == 17, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultModSpecParams[17] = {
      0, // M1en = 0
      0, // M1src = 0
      0, // M1dest1 = 0
      0, // M1dest2 = 0
      0, // M1dest3 = 0
      0, // M1dest4 = 0
      24575, // M1scl1 = 0.75
      24575, // M1scl2 = 0.75
      24575, // M1scl3 = 0.75
      24575, // M1scl4 = 0.75
      0, // M1Aen = 0
      0, // M1Asrc = 0
      32767, // M1Aatt = 1
      10922, // M1rngA = 0.3333333432674407959
      21844, // M1rngB = 0.6666666865348815918
      16383, // M1rngXA = 0.5
      21844, // M1rngXB = 0.6666666865348815918
    };
    static_assert((int)M7::LFOParamIndexOffsets::Count == 7, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultLFOParams[7] = {
      15, // LFO1wav = 0.00048828125
      16383, // LFO1shA = 0.5
      16383, // LFO1shB = 0.5
      0, // LFO1rst = 0
      0, // LFO1ph = 0
      19660, // LFO1fr = 0.60000002384185791016
      16383, // LFO1lp = 0.5
    };
    static_assert((int)M7::EnvParamIndexOffsets::Count == 11, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultEnvelopeParams[11] = {
      0, // AE1dlt = 0
      1638, // AE1att = 0.050000000745058059692
      16383, // AE1atc = 0.5
      0, // AE1ht = 0
      16383, // AE1dt = 0.5
      -16383, // AE1dc = -0.5
      13106, // AE1sl = 0.40000000596046447754
      6553, // AE1rt = 0.20000000298023223877
      -16383, // AE1tc = -0.5
      32767, // AE1rst = 1
      0, // AE1mode = 0
    };
    static_assert((int)M7::OscParamIndexOffsets::Count == 19, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultOscillatorParams[19] = {
      0, // O1En = 0
      32767, // O1Vol = 1
      0, // O1Pan = 0
      0, // O1KRmin = 0
      507, // O1KRmax = 0.0155029296875
      0, // O1Wave = 0
      0, // O1ShpA = 0
      16383, // O1ShpB = 0.5
      0, // O1PRst = 0
      0, // O1Poff = 0
      0, // O1Scen = 0
      9830, // O1ScFq = 0.30000001192092895508
      32767, // O1ScKt = 1
      9830, // O1Fq = 0.30000001192092895508
      32767, // O1FqKt = 1
      0, // O1Semi = 0
      0, // O1Fine = 0
      511, // O1Mul = 0.015625
      0, // O1FMFb = 0
    };
    static_assert((int)M7::FilterParamIndexOffsets::Count == 5, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultFilterParams[5] = {
      0, // F1En = 0
      0, // F1Resp = 0
      6553, // F1Q = 0.20000000298023223877
      9830, // F1Freq = 0.30000001192092895508
      32767, // F1FKT = 1
    };
  } // namespace M7
} // namespace WaveSabreCore
