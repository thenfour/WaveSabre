
#include "Maj7Vst.h"
#include "Maj7Editor.h"

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	Helpers::Init();
	return new Maj7Vst(audioMaster);
}

Maj7Vst::Maj7Vst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)M7::Maj7::ParamIndices::NumParams, 0, 2, 'maj7', new M7::Maj7(), true)
{
	setEditor(new Maj7Editor(this));
}

void Maj7Vst::getParameterName(VstInt32 index, char* text)
{
	static constexpr std::pair< M7::Maj7::ParamIndices, const char*> nameMap[] = {
		{M7::Maj7::ParamIndices::MasterVolume, "Master"},
		{M7::Maj7::ParamIndices::VoicingMode, "PolyMon"},
		{M7::Maj7::ParamIndices::Unisono, "Unisono"},
		{M7::Maj7::ParamIndices::UnisonoDetune, "UniDet"},
		{M7::Maj7::ParamIndices::OscillatorDetune, "OscDet"},
		{M7::Maj7::ParamIndices::StereoSpread, "Spread"},
		{M7::Maj7::ParamIndices::FMBrightness, "FMBrigh"},

		{M7::Maj7::ParamIndices::Macro1, "Macro1"},
		{M7::Maj7::ParamIndices::Macro2, "Macro2"},
		{M7::Maj7::ParamIndices::Macro3, "Macro3"},
		{M7::Maj7::ParamIndices::Macro4, "Macro4"},

		{M7::Maj7::ParamIndices::PortamentoTime, "PortTm"},
		{M7::Maj7::ParamIndices::PortamentoCurve, "PortCv"},
		{M7::Maj7::ParamIndices::PitchBendRange, "PBRng"},

		{M7::Maj7::ParamIndices::Osc1Enabled, "O1En"},
		{M7::Maj7::ParamIndices::Osc1Volume, "O1Vol"},
		{M7::Maj7::ParamIndices::Osc1Waveform, "O1Wave"},
		{M7::Maj7::ParamIndices::Osc1Waveshape, "O1Shp"},
		{M7::Maj7::ParamIndices::Osc1PhaseRestart, "O1PRst"},
		{M7::Maj7::ParamIndices::Osc1PhaseOffset, "O1Poff"},
		{M7::Maj7::ParamIndices::Osc1SyncEnable, "O1Scen"},
		{M7::Maj7::ParamIndices::Osc1SyncFrequency, "O1ScFq"},
		{M7::Maj7::ParamIndices::Osc1SyncFrequencyKT, "O1ScKt"},
		{M7::Maj7::ParamIndices::Osc1FrequencyParam, "O1Fq"},
		{M7::Maj7::ParamIndices::Osc1FrequencyParamKT, "O1FqKt"},
		{M7::Maj7::ParamIndices::Osc1PitchSemis, "O1Semi"},
		{M7::Maj7::ParamIndices::Osc1PitchFine, "O1Fine"},
		{M7::Maj7::ParamIndices::Osc1FreqMul, "O1Mul"},
		{M7::Maj7::ParamIndices::Osc1FMFeedback, "O1FMFb"},

		{M7::Maj7::ParamIndices::Osc2Enabled, "O2En"},
		{M7::Maj7::ParamIndices::Osc2Volume, "O2Vol"},
		{M7::Maj7::ParamIndices::Osc2Waveform, "O2Wave"},
		{M7::Maj7::ParamIndices::Osc2Waveshape, "O2Shp"},
		{M7::Maj7::ParamIndices::Osc2PhaseRestart, "O2PRst"},
		{M7::Maj7::ParamIndices::Osc2PhaseOffset, "O2Poff"},
		{M7::Maj7::ParamIndices::Osc2SyncEnable, "O2Scen"},
		{M7::Maj7::ParamIndices::Osc2SyncFrequency, "O2ScFq"},
		{M7::Maj7::ParamIndices::Osc2SyncFrequencyKT, "O2ScKt"},
		{M7::Maj7::ParamIndices::Osc2FrequencyParam, "O2Fq"},
		{M7::Maj7::ParamIndices::Osc2FrequencyParamKT, "O2FqKt"},
		{M7::Maj7::ParamIndices::Osc2PitchSemis, "O2Semi"},
		{M7::Maj7::ParamIndices::Osc2PitchFine, "O2Fine"},
		{M7::Maj7::ParamIndices::Osc2FreqMul, "O2Mul"},
		{M7::Maj7::ParamIndices::Osc2FMFeedback, "O2FMFb"},

		{M7::Maj7::ParamIndices::Osc3Enabled, "O3En"},
		{M7::Maj7::ParamIndices::Osc3Volume, "O3Vol"},
		{M7::Maj7::ParamIndices::Osc3Waveform, "O3Wave"},
		{M7::Maj7::ParamIndices::Osc3Waveshape, "O3Shp"},
		{M7::Maj7::ParamIndices::Osc3PhaseRestart, "O3PRst"},
		{M7::Maj7::ParamIndices::Osc3PhaseOffset, "O3Poff"},
		{M7::Maj7::ParamIndices::Osc3SyncEnable, "O3Scen"},
		{M7::Maj7::ParamIndices::Osc3SyncFrequency, "O3ScFq"},
		{M7::Maj7::ParamIndices::Osc3SyncFrequencyKT, "O3ScKt"},
		{M7::Maj7::ParamIndices::Osc3FrequencyParam, "O3Fq"},
		{M7::Maj7::ParamIndices::Osc3FrequencyParamKT, "O3FqKt"},
		{M7::Maj7::ParamIndices::Osc3PitchSemis, "O3Semi"},
		{M7::Maj7::ParamIndices::Osc3PitchFine, "O3Fine"},
		{M7::Maj7::ParamIndices::Osc3FreqMul, "O3Mul"},
		{M7::Maj7::ParamIndices::Osc3FMFeedback, "O3FMFb"},

		{M7::Maj7::ParamIndices::AmpEnvDelayTime, "AEdlt"},
		{M7::Maj7::ParamIndices::AmpEnvAttackTime, "AEatt"},
		{M7::Maj7::ParamIndices::AmpEnvAttackCurve, "AEatc"},
		{M7::Maj7::ParamIndices::AmpEnvHoldTime, "AEht"},
		{M7::Maj7::ParamIndices::AmpEnvDecayTime, "AEdt"},
		{M7::Maj7::ParamIndices::AmpEnvDecayCurve, "AEdc"},
		{M7::Maj7::ParamIndices::AmpEnvSustainLevel, "AEsl"},
		{M7::Maj7::ParamIndices::AmpEnvReleaseTime, "AErt"},
		{M7::Maj7::ParamIndices::AmpEnvReleaseCurve, "AEtc"},
		{M7::Maj7::ParamIndices::AmpEnvLegatoRestart, "AErst"},

		{M7::Maj7::ParamIndices::Env1DelayTime, "E1dlt"},
		{M7::Maj7::ParamIndices::Env1AttackTime, "E1att"},
		{M7::Maj7::ParamIndices::Env1AttackCurve, "E1atc"},
		{M7::Maj7::ParamIndices::Env1HoldTime, "E1ht"},
		{M7::Maj7::ParamIndices::Env1DecayTime, "E1dt"},
		{M7::Maj7::ParamIndices::Env1DecayCurve, "E1dc"},
		{M7::Maj7::ParamIndices::Env1SustainLevel, "E1sl"},
		{M7::Maj7::ParamIndices::Env1ReleaseTime, "E1rt"},
		{M7::Maj7::ParamIndices::Env1ReleaseCurve, "E1tc"},
		{M7::Maj7::ParamIndices::Env1LegatoRestart, "E1rst"},

		{M7::Maj7::ParamIndices::Env2DelayTime, "E2dlt"},
		{M7::Maj7::ParamIndices::Env2AttackTime, "E2att"},
		{M7::Maj7::ParamIndices::Env2AttackCurve, "E2atc"},
		{M7::Maj7::ParamIndices::Env2HoldTime, "E2ht"},
		{M7::Maj7::ParamIndices::Env2DecayTime, "E2dt"},
		{M7::Maj7::ParamIndices::Env2DecayCurve, "E2dc"},
		{M7::Maj7::ParamIndices::Env2SustainLevel, "E2sl"},
		{M7::Maj7::ParamIndices::Env2ReleaseTime, "E2rt"},
		{M7::Maj7::ParamIndices::Env2ReleaseCurve, "E2tc"},
		{M7::Maj7::ParamIndices::Env2LegatoRestart, "E2rst"},

		{M7::Maj7::ParamIndices::LFO1Waveform, "LFO1wav"},
		{M7::Maj7::ParamIndices::LFO1Waveshape, "LFO1shp"},
		{M7::Maj7::ParamIndices::LFO1Restart, "LFO1rst"},
		{M7::Maj7::ParamIndices::LFO1PhaseOffset, "LFO1ph"},
		{M7::Maj7::ParamIndices::LFO1FrequencyParam, "LFO1fr"},

		{M7::Maj7::ParamIndices::LFO2Waveform, "LFO2wav"},
		{M7::Maj7::ParamIndices::LFO2Waveshape, "LFO2shp"},
		{M7::Maj7::ParamIndices::LFO2Restart, "LFO2rst"},
		{M7::Maj7::ParamIndices::LFO2PhaseOffset, "LFO2ph"},
		{M7::Maj7::ParamIndices::LFO2FrequencyParam, "LFO2fr"},

		{M7::Maj7::ParamIndices::FilterType, "FLtype"},
		{M7::Maj7::ParamIndices::FilterQ, "FLq"},
		{M7::Maj7::ParamIndices::FilterSaturation, "FLsat"},
		{M7::Maj7::ParamIndices::FilterFrequency, "FLfreq"},
		{M7::Maj7::ParamIndices::FilterFrequencyKT, "FLkt"},

		{M7::Maj7::ParamIndices::FMAmt1to2, "FM1to2"},
		{M7::Maj7::ParamIndices::FMAmt1to3, "FM1to3"},
		{M7::Maj7::ParamIndices::FMAmt2to1, "FM2to1"},
		{M7::Maj7::ParamIndices::FMAmt2to3, "FM2to3"},
		{M7::Maj7::ParamIndices::FMAmt3to1, "FM3to1"},
		{M7::Maj7::ParamIndices::FMAmt3to2, "FM3to2"},

		{M7::Maj7::ParamIndices::Mod1Enabled, "M1en"},
		{M7::Maj7::ParamIndices::Mod1Source, "M1src"},
		{M7::Maj7::ParamIndices::Mod1Destination, "M1dest"},
		{M7::Maj7::ParamIndices::Mod1Curve, "M1curv"},
		{M7::Maj7::ParamIndices::Mod1Scale, "M1scale"},

		{M7::Maj7::ParamIndices::Mod2Enabled, "M2en"},
		{M7::Maj7::ParamIndices::Mod2Source, "M2src"},
		{M7::Maj7::ParamIndices::Mod2Destination, "M2dest"},
		{M7::Maj7::ParamIndices::Mod2Curve, "M2curv"},
		{M7::Maj7::ParamIndices::Mod2Scale, "M2scale"},

		{M7::Maj7::ParamIndices::Mod3Enabled, "M3en"},
		{M7::Maj7::ParamIndices::Mod3Source, "M3src"},
		{M7::Maj7::ParamIndices::Mod3Destination, "M3dest"},
		{M7::Maj7::ParamIndices::Mod3Curve, "M3curv"},
		{M7::Maj7::ParamIndices::Mod3Scale, "M3scale"},

		{M7::Maj7::ParamIndices::Mod4Enabled, "M4en"},
		{M7::Maj7::ParamIndices::Mod4Source, "M4src"},
		{M7::Maj7::ParamIndices::Mod4Destination, "M4dest"},
		{M7::Maj7::ParamIndices::Mod4Curve, "M4curv"},
		{M7::Maj7::ParamIndices::Mod4Scale, "M4scale"},

		{M7::Maj7::ParamIndices::Mod5Enabled, "M5en"},
		{M7::Maj7::ParamIndices::Mod5Source, "M5src"},
		{M7::Maj7::ParamIndices::Mod5Destination, "M5dest"},
		{M7::Maj7::ParamIndices::Mod5Curve, "M5curv"},
		{M7::Maj7::ParamIndices::Mod5Scale, "M5scale"},

		{M7::Maj7::ParamIndices::Mod6Enabled, "M6en"},
		{M7::Maj7::ParamIndices::Mod6Source, "M6src"},
		{M7::Maj7::ParamIndices::Mod6Destination, "M6dest"},
		{M7::Maj7::ParamIndices::Mod6Curve, "M6curv"},
		{M7::Maj7::ParamIndices::Mod6Scale, "M6scale"},

		{M7::Maj7::ParamIndices::Mod7Enabled, "M7en"},
		{M7::Maj7::ParamIndices::Mod7Source, "M7src"},
		{M7::Maj7::ParamIndices::Mod7Destination, "M7dest"},
		{M7::Maj7::ParamIndices::Mod7Curve, "M7curv"},
		{M7::Maj7::ParamIndices::Mod7Scale, "M7scale"},

		{M7::Maj7::ParamIndices::Mod8Enabled, "M8en"},
		{M7::Maj7::ParamIndices::Mod8Source, "M8src"},
		{M7::Maj7::ParamIndices::Mod8Destination, "M8dest"},
		{M7::Maj7::ParamIndices::Mod8Curve, "M8curv"},
		{M7::Maj7::ParamIndices::Mod8Scale, "M8scale"},
	};
	static_assert(std::size(nameMap) == (int)M7::Maj7::ParamIndices::NumParams, "You're missing param names");
	for (int i = 0; i < std::size(nameMap); ++i) {
		char b[200];
		sprintf(b, "param names must be in order; %d != %d (%s)", nameMap[i].first, i, nameMap[i].second);
		//WSAssert((int)nameMap[i].first == i, b);
		//WSAssert(strlen(nameMap[i].second) < kVstMaxParamStrLen, "param name too long");
	}
	if (index < 0 || index >= std::size(nameMap)) return;
	vst_strncpy(text, nameMap[index].second, kVstMaxParamStrLen);
}

bool Maj7Vst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Maj7", kVstMaxEffectNameLen);
	return true;
}

bool Maj7Vst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Maj7", kVstMaxProductStrLen);
	return true;
}

M7::Maj7* Maj7Vst::GetMaj7() const
{
	return (M7::Maj7*)getDevice();
}
