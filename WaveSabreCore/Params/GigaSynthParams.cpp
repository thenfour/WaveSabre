#include "../GigaSynth/GigaParams.hpp"
namespace WaveSabreCore {
  namespace M7 {
    static_assert((int)M7::MainParamIndices::Count == 29, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultMasterParams[29] = {
      16422, // Master = 0.50118726491928100586
      16384, // Pan = 0.5
      0, // PolyMon = 0
      8, // Unisono = 0.000244140625
      0, // UniDet = 0
      0, // UniSpr = 0
      16384, // FMBrigh = 0.5
      9830, // PortTm = 0.30000001192092895508
      16, // PBRng = 0.00048828125
      192, // MaxVox = 0.005859375
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
    static_assert((int)M7::SamplerParamIndexOffsets::Count == 23, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultSamplerParams[23] = {
      0, // S1En = 0
      32767, // S1Vol = 1
      16384, // S1Pan = 0.5
      16422, // S1CGain = 0.50118720531463623047
      0, // S1KRmin = 0
      1016, // S1KRmax = 0.031005859375
      480, // S1base = 0.0146484375
      32767, // S1LTrig = 1
      0, // S1Rev = 0
      8, // S1src = 0.000244140625
      0, // S1gmidx = 0
      0, // S1strt = 0
      8, // S1LMode = 0.000244140625
      0, // S1LSrc = 0
      0, // S1Lbeg = 0
      32767, // S1Llen = 1
      0, // S1TunS = 0
      16384, // S1TunF = 0.5
      9830, // S1Frq = 0.30000001192092895508
      32767, // S1FrqKT = 1
      8, // S1Intrp = 0.000244140625
      32767, // S1RelX = 1
      0, // S1Dly = 0
    };
    static_assert((int)M7::ModParamIndexOffsets::Count == 19, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultModSpecParams[19] = {
      0, // M1en = 0
      0, // M1src = 0
      0, // M1dest1 = 0
      0, // M1dest2 = 0
      0, // M1dest3 = 0
      0, // M1dest4 = 0
      16384, // M1curv = 0.5
      28672, // M1scl1 = 0.875
      28672, // M1scl2 = 0.875
      28672, // M1scl3 = 0.875
      28672, // M1scl4 = 0.875
      0, // M1Aen = 0
      0, // M1Asrc = 0
      32767, // M1Aatt = 1
      16384, // M1Acrv = 0.5
      10922, // M1rngA = 0.3333333432674407959
      21845, // M1rngB = 0.6666666865348815918
      16384, // M1rngXA = 0.5
      21845, // M1rngXB = 0.6666666865348815918
    };
    static_assert((int)M7::LFOParamIndexOffsets::Count == 12, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultLFOParams[12] = {
      32, // LFO1wav = 0.0009765625
      16384, // LFO1shA = 0.5
      16384, // LFO1shB = 0.5
      0, // LFO1rst = 0
      16384, // LFO1ph = 0.5
      19660, // LFO1fr = 0.60000002384185791016
      0, // LFO1TmB = 0
      24, // LFO1Num = 0.000732421875
      8, // LFO1Den = 0.000244140625
      0, // LFO1F8s = 0
      16384, // LFO1ms = 0.5
      16384, // LFO1lp = 0.5
    };
    static_assert((int)M7::EnvParamIndexOffsets::Count == 11, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultEnvelopeParams[11] = {
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
    static_assert((int)M7::OscParamIndexOffsets::Count == 20, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultOscillatorParams[20] = {
      0, // O1En = 0
      32767, // O1Vol = 1
      16384, // O1Pan = 0.5
      16422, // O1CGain = 0.50118720531463623047
      0, // O1KRmin = 0
      1016, // O1KRmax = 0.031005859375
      0, // O1Wave = 0
      0, // O1ShpA = 0
      16384, // O1ShpB = 0.5
      0, // O1PRst = 0
      16384, // O1Poff = 0.5
      0, // O1Scen = 0
      9830, // O1ScFq = 0.30000001192092895508
      32767, // O1ScKt = 1
      9830, // O1Fq = 0.30000001192092895508
      32767, // O1FqKt = 1
      0, // O1Semi = 0
      16384, // O1Fine = 0.5
      512, // O1Mul = 0.015625
      0, // O1FMFb = 0
    };
    static_assert((int)M7::FilterParamIndexOffsets::Count == 7, "param count probably changed and this needs to be regenerated.");
    extern const int16_t gDefaultFilterParams[7] = {
      0, // F1En = 0
      32, // F1Circ = 0.0009765625
      24, // F1Slop = 0.000732421875
      0, // F1Resp = 0
      6553, // F1Q = 0.20000000298023223877
      9830, // F1Freq = 0.30000001192092895508
      32767, // F1FKT = 1
    };
  } // namespace M7
} // namespace WaveSabreCore
