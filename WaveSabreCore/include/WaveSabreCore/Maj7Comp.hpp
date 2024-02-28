
#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"

/*

slider1:-20<-60,0,1>Threshold
slider2:60<0,1000>Attack
slider3:80<0,2000>Release
slider4:4<1,60>Ratio
slider5:0<0,30>Knee
slider8:0<0,1,1{Stereo,M/S}>Channel Processing
slider9:80<0,100>Channel Link
// output gain is not the same as gain compensation. gain compensation aims to equalize the output with the input,
// while output gain should aim to be 0.
// the purpose behind this is that wet/dry parallel mixing is done after gain compensation, but before output gain.
slider10:0<-36,36>Output gain
slider11:sInputGainDB=0<-36,36>Input gain
slider12:sGainCompensationDB=0<-36,36>Gain compensation
slider13:sDryWet01=1<0,1>Dry-wet(parallel)

slider14:sPeakRMSMix=0<0,1>Peak<->RMS detection
// a slow default because if you're selecting RMS it's likely that you're looking for a smooth slow response. give it.
slider15:sRMSWindowMS=30<3,50>RMS window(ms)

slider17:sHPF=110<20,2000>HP

slider30:sOutputSignal=0<0,5,1{Normal,Diff,Sidechain,GainReduction,RMSDetector,Detector}>Output

*/


namespace WaveSabreCore
{
	struct Maj7Comp: public Device
	{
		enum class ParamIndices
		{
			InputGain, // db -inf to +24
			Threshold, // db -60 to 0, default -20
			Attack, // 0-500ms, default 60
			Release, // 0-1000ms, default 80
			Ratio, // 1-40, default 4
			Knee, // 1-30, default 8
			MidSideEnable, // stereo or mid/side, default stereo
			ChannelLink, // 0-100%, default 80%
			CompensationGain, // -inf to +24db, default 0
			DryWet,// mix 0-100%, default 100%
			PeakRMSMix, // 0-100%, default 0 (peak)
			RMSWindow, // 3-50ms, default 30ms
			HighPassFrequency, // biquad freq; default lo
			HighPassQ, // default 0.2
			LowPassFrequency, // biquad freq; default hi
			LowPassQ, // default 0.2
			OutputSignal, // -inf to +24
			OutputGain, // -inf to +24
			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7COMP_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Comp::ParamIndices::NumParams]{ \
	{"InpGain"},\
	{"Thresh"},\
	{"Attack"},\
	{"Release"},\
	{"Ratio"},\
	{"Knee"},\
	{"MSEnable"},\
	{"ChanLink"},\
	{"CompGain"},\
	{"DryWet"},\
	{"PeakRMS"},\
	{"RMSMS"},\
	{"HPF"},\
	{"HPQ"},\
	{"LPF"},\
	{"LPQ"},\
	{"OutSig"},\
	{"OutGain"},\
}

		static_assert((int)ParamIndices::NumParams == 18, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[18] = {
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		  0,
		};

		float mParamCache[(int)ParamIndices::NumParams];

		M7::ParamAccessor mParams;

		Maj7Comp() :
			Device((int)ParamIndices::NumParams),
			mParams(mParamCache, 0)
		{
		}

		virtual void LoadDefaults() override
		{
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				outputs[0][i] = inputs[0][i];
				outputs[1][i] = inputs[1][i];
			}
		}
	};
}




