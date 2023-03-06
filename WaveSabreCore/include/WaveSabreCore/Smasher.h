#ifndef __WAVESABRECORE_SMASHER_H__
#define __WAVESABRECORE_SMASHER_H__

#include "Device.h"
#include "DelayBuffer.h"

namespace WaveSabreCore
{
	class Smasher : public Device
	{
	public:
		enum class ParamIndices
		{
			Sidechain,
			InputGain,
			Threshold,
			Ratio,
			Attack,
			Release,
			OutputGain,

			NumParams,
		};

		Smasher();
		virtual ~Smasher();

		virtual void Run(double songPosition, float **inputs, float **outputs, int numSamples);

		virtual void SetParam(int index, float value);
		virtual float GetParam(int index) const;

		float gainScalar; // compression gain
		float inputLevel;  // input peak level
		float peak; // peak used for compression trig
		float outputPeak; // level
		float atten;
		float thresholdScalar;

	private:
		const static float lookaheadMs;

		bool sidechain;
		float threshold; // DB
		float inputGain;
		float ratio;
		float attack, release;
		float outputGain;

		DelayBuffer leftBuffer;
		DelayBuffer rightBuffer;
	};
}

#endif
