// see https://github.com/buosseph/juce-distortion/blob/master/Source/Distortion.cpp
// https://github.com/search?q=foldback+language%3AC%2B%2B&type=code

#include <WaveSabreCore/Scissor.h>
#include <WaveSabreCore/Helpers.h>
#include "WaveSabreCore/Maj7Basic.hpp"

// type (smin, tanh)
// drive
// shape
// harmonic
// oversampling


namespace WaveSabreCore
{
	Scissor::Scissor()
		: Device(1)
	{
		//type = ShaperType::Clipper;
		//drive = .2f;
		//threshold = .8f;
		//foldover = 0.0f;
		//dryWet = 1.0f;
		//oversampling = Oversampling::X1;

		//lastSample[0] = lastSample[1] = 0.0f;
	}


	void Scissor::SetParam(int index, float value)
	{
	}

	float Scissor::GetParam(int index) const
	{
		return 0;
	}

	void Scissor::Run(double songPosition, float **inputs, float **outputs, int numSamples)
	{
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < numSamples; j++)
			{
				outputs[i][j] = inputs[i][j];
			}
		}
	}
}
