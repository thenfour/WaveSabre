
#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"

namespace WaveSabreCore
{


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Maj7Sat : public Device
	{
		//static constexpr float CalcDivK(float alpha) {
		//	return 2.0f * alpha / (1.0f - alpha);
		//}
		// for the div style shaping, `k+1` also happens to represent the slope
		static constexpr float gK41 = 1.38983050847f; // CalcDivK(0.41f)
		static constexpr float gK63 = 3.40540540541f;
		static constexpr float gK85 = 11.3333333333f;

		static constexpr float gAnalogMaxLin = 16;

		enum class PanMode : uint8_t {
			Stereo,
			MidSide,
			Count__,
		};

		enum class Oversampling : uint8_t {
			Off,
			x2,
			x4,
			x8,
			Count__,
		};

		enum class Model : uint8_t {
			Thru,
			SineClip,
			DivClipSoft,
			TanhClip,
			DivClipMedium,
			DivClipHard,
			HardClip,
			TanhFold,
			TanhSinFold,
			SinFold,
			LinearFold,
			Count__,
		};

		// helps unify the sound of them. nothing magic here just values that seemed to work.
		// the method is to just keep a fundamental frequency constant, and let the harmonics add.
		// so using a fft meter to match a sine wave fundamental level.
		static constexpr float ModelPregain[(size_t)Model::Count__] = {
			1, // thru,
			0.71f, // sinclip
			0.69f, // div 41
			0.62f, // tanh clip
			0.47f, // DivClipMedium,
			0.21f, // DivClipHard,
			1, // HardClip,
			0.41f,// TanhFold,
			0.56f,// TanhSinFold,
			0.71f,// SinFold,
			1, // LinearFold,
		};

		// to calculate the slope...
		//z = 0.000001;
		//alpha = .85;
		//calcslope =(shape_ws1(z,alpha) - shape_ws1(0,alpha)) / z;
		//calcslope =(shape_sinclip(z) - shape_sinclip(-z)) / (2*z);
		//calcslope =(shape_tanh(z) - shape_tanh(0)) / z;
		//calcslope =(shape_hardclip(z) - shape_hardclip(0)) / z;
		static constexpr float ModelNaturalSlopes[(size_t)Model::Count__] = {
			1, // thru,
			M7::math::gPIHalf, // sinclip
			gK41 + 1, // div 41
			2, // tanh clip
			gK63 + 1, // DivClipMedium,
			gK85 + 1, // DivClipHard,
			1, // HardClip,
			M7::math::gPI,// TanhFold, <-- really?
			M7::math::gPI * 3.0f / 4.0f,// TanhSinFold,
			M7::math::gPIHalf,// SinFold,
			1, // LinearFold,
		};

		enum class OutputSignal : uint8_t {
			WetCombined,
			WetBandLow,
			WetBandMid,
			WetBandHigh,
			DryCombined,
			DryBandLow,
			DryBandMid,
			DryBandHigh,
			Count__,
		};

		enum class ParamIndices
		{
			InputGain,
			CrossoverAFrequency,
			CrossoverASlope,
			CrossoverBFrequency,

			Oversampling,
			OutputGain,
			OutputSignal,

			// low band
			AEnable,
			APanMode,
			APan,
			AModel,
			ADrive,
			ACompensationGain,
			AThreshold,
			AEvenHarmonics,
			ADryWet,

			// mid band
			BEnable,
			BPanMode,
			BPan,
			BModel,
			BDrive,
			BCompensationGain,
			BThreshold,
			BEvenHarmonics,
			BDryWet,

			// hi band
			CEnable,
			CPanMode,
			CPan,
			CModel,
			CDrive,
			CCompensationGain,
			CThreshold,
			CEvenHarmonics,
			CDryWet,

			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7SAT_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Sat::ParamIndices::NumParams]{ \
			{"InpGain"}, /* InputGain */ \
			{"xAFreq"}, /* CrossoverAFrequency */ \
			{"xASlope"}, /* CrossoverASlope */ \
			{"xBFreq"}, /* CrossoverBFrequency */ \
		{"Oversamp"}, /* Oversampling */ \
			{"OutpGain"}, /* OutputGain */ \
			{"OutpSig"}, /* OutputSignal */ \
		{"0Enable"}, /* enable */ \
		{"0PanMode"}, /* PanMode */ \
		{"0Pan"}, /* Pan */ \
		{"0Model"}, /* Model */ \
		{"0Drive"}, /* Drive */ \
		{"0CmpGain"}, /* CompensationGain */ \
		{"0Thresh"}, /* Threshold */ \
		{"0Analog"}, /* EvenHarmonics */ \
		{"0DryWet"}, /* DryWet */ \
		{"1Enable"}, /* enable */ \
		{"1PanMode"}, /* PanMode */ \
		{"1Pan"}, /* Pan */ \
		{"1Model"}, /* Model */ \
		{"1Drive"}, /* Drive */ \
		{"1CmpGain"}, /* CompensationGain */ \
		{"1Thresh"}, /* Threshold */ \
		{"1Analog"}, /* EvenHarmonics */ \
		{"2Enable"}, /* enable */ \
		{"1DryWet"}, /* DryWet */ \
		{"2PanMode"}, /* PanMode */ \
		{"2Pan"}, /* Pan */ \
		{"2Model"}, /* Model */ \
		{"2Drive"}, /* Drive */ \
		{"2CmpGain"}, /* CompensationGain */ \
		{"2Thresh"}, /* Threshold */ \
		{"2Analog"}, /* EvenHarmonics */ \
		{"2DryWet"}, /* DryWet */ \
}

		static_assert((int)ParamIndices::NumParams == 34, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[34] = {
		  8230, // InpGain = 0.25118863582611083984
		  12052, // xAFreq = 0.36780720949172973633
		  8, // xASlope = 0.0002470355830155313015
		  20715, // xBFreq = 0.63219285011291503906
		  -32768, // Oversamp = -1
		  8230, // OutpGain = 0.25118863582611083984
		  72, // OutpSig = 0.002197265625

		  0, // enable
		  -32768, // PanMode = -1
		  16384, // Pan = 0.5
		  24, // Model = 0.000732421875
		  4125, // Drive = 0.12589254975318908691
		  8230, // CompGain = 0.25118863582611083984
		  20675, // Thresh = 0.63095736503601074219
		  4125, // Analog = 0.12589254975318908691
		  32767, // DryWet = 1

		  0, // enable
		  -32768, // PanMode = -1
		  16384, // Pan = 0.5
		  24, // Model = 0.000732421875
		  4125, // Drive = 0.12589254975318908691
		  8230, // CompGain = 0.25118863582611083984
		  20675, // Thresh = 0.63095736503601074219
		  4125, // Analog = 0.12589254975318908691
		  32767, // DryWet = 1

		  0, // enable
		  -32768, // PanMode = -1
		  16384, // Pan = 0.5
		  24, // Model = 0.000732421875
		  4125, // Drive = 0.12589254975318908691
		  8230, // CompGain = 0.25118863582611083984
		  20675, // Thresh = 0.63095736503601074219
		  4125, // Analog = 0.12589254975318908691
		  32767, // DryWet = 1
		};

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct FreqBand {
			enum class BandParam {
				Enable,
				PanMode,
				Pan,
				Model,
				Drive,
				CompensationGain,
				Threshold,
				EvenHarmonics,
				DryWet,
			};

			float mDriveLin = 0;
			float mCompensationGain = 0;
			float mAutoDriveCompensation = 0; // driveCompensation
			float mThresholdLin = 0;
			float mDryWet = 0;
			float mEvenHarmonicsGainLin = 0; // analogMix
			float mPanN11 = 0;
			float mCorrSlope = 0;

			float output0 = 0;
			float output1 = 0;

			Model mModel = Model::Thru;
			PanMode mPanMode = PanMode::Stereo;
			bool mEnable = false;

			M7::ParamAccessor mParams;
			M7::DCFilter mDC[2];

			FreqBand(float* paramCache, ParamIndices baseParamID) : //
				mParams(paramCache, baseParamID)
			{}

			void Slider() {

				mEnable = mParams.GetBoolValue(BandParam::Enable);
				mDriveLin = mParams.GetLinearVolume(BandParam::Drive, M7::gVolumeCfg36db);
				mCompensationGain = mParams.GetLinearVolume(BandParam::CompensationGain, M7::gVolumeCfg24db);
				mModel = mParams.GetEnumValue<Model>(BandParam::Model);

				mThresholdLin = mParams.GetLinearVolume(BandParam::Threshold, M7::gUnityVolumeCfg);
				// threshold must be corrected, otherwise it will cause a div by 0 when we scale the waveform up.
				mThresholdLin = M7::math::clamp(mThresholdLin, 0, 0.99f);

				mDryWet = mParams.Get01Value(BandParam::DryWet, 0);
				mEvenHarmonicsGainLin = mParams.GetLinearVolume(BandParam::EvenHarmonics, M7::gVolumeCfg36db, 0);

				mPanMode = mParams.GetEnumValue<PanMode>(BandParam::PanMode);
				mPanN11 = mParams.GetN11Value(BandParam::Pan, 0);

				static constexpr float dck = 1.5f; // this controls how extreme the compensation is. this feels about right.
				//but in theory it depends on the input signal; some plugins do auto gain compensation by comparing RMS... i'm not doing that.
				mAutoDriveCompensation = 1.0f / (1.0f + dck * M7::math::log10(std::max(1.0f, mDriveLin))); // log base e? or 10?

				mCorrSlope = M7::math::lerp(ModelNaturalSlopes[(int)mModel], 1, mThresholdLin);
			}
			// Patrice Tarrabia and Bram de Jong (supposedly? can't find it anywhere)
// but it's very simple and works very well, sounds good, has a shape parameter
			static float shape_div(float sample, float k)
			{
				return (1.0f + k) * sample / (1.0f + k * sample);
			}

			static float shape_sinclip(float s)
			{
				return M7::math::sin(M7::math::gPIHalf * M7::math::clamp01(s));
			}

			static float shape_tanhclip(float in)
			{
				return M7::math::tanh(in * 2);
			}

			static float shape_hardclip(float in)
			{
				// hard-clipping at 1 doesn't make sense because it renders threshold pointless.
				// hard-clipping at 0.5 does a good job of keeping loudness the same and also giving meaning to threshold.
				return std::min(in, 0.5f);
			}

			static float shape_sinfold(float in)
			{
				return M7::math::sin(M7::math::gPIHalf * in);
			}

			static float shape_tanhfold(float in)
			{
				return M7::math::tanh(2 * M7::math::sin(in * M7::math::gPIHalf));
			}

			// this is a sharp "naive" linear folding shape. i find it funny, because
			// 1. even though it's just "linear", it means having to use a triangle wave to fold it back and forth, which is more code than sine()
			// 2. even though it's more directly related to the incoming wave, it sounds very noisy and harsh compared to other folding shapes.
			float shape_trifold(float s)
			{
				s = .25f * s - .25f;
				float b = M7::math::floor(s);
				s = std::abs(2 * (s - b - .5f));
				s = 2 * s - 1;
				return s;
			}

			static float shape_tanhSinFold(float in)
			{
				return M7::math::tanh(1.5f * M7::math::sin(in * M7::math::gPIHalf));
			}

			float distort(float s, size_t chanIndex)
			{
				s *= ModelPregain[(int)mModel];

				s *= mDriveLin;

				float dryTemp = s;
				float g = s < 0 ? -1.0f : 1.0f;
				s = std::abs(s); // work only in positive pole for shaping.

				// at thresh=1, no shaping occurs and slope is 1 (linear).
				// at thresh=0, the whole curve is shaped so gets the natural slope of the model = pi/2.
				s *= mCorrSlope;

				if (s > mThresholdLin) {
					// we will be shaping the area above thresh, so map (thresh,1) to (0,1)
					s -= mThresholdLin;
					s /= (1.0f - mThresholdLin);
					s /= ModelNaturalSlopes[(int)mModel];

					// perform shaping
					switch (mModel) {
					default:
					case Model::Thru:
						// nop
						break;
					case Model::SineClip:
						s = shape_sinclip(s);
						break;
					case Model::DivClipSoft:
						s = shape_div(s, gK41);
						break;
					case Model::TanhClip:
						s = shape_tanhclip(s);
						break;
					case Model::DivClipMedium:
						s = shape_div(s, gK63);
						break;
					case Model::DivClipHard:
						s = shape_div(s, gK85);
						break;
					case Model::HardClip:
						s = shape_hardclip(s);
						break;
					case Model::TanhFold:
						s = shape_tanhfold(s);
						break;
					case Model::TanhSinFold:
						s = shape_tanhSinFold(s);
						break;
					case Model::SinFold:
						s = shape_sinfold(s);
						break;
					case Model::LinearFold:
						s = shape_trifold(s);
						break;
					}

					// now map back (0,1) to (thresh,1).
					s *= (1.0f - mThresholdLin);
					s += mThresholdLin;
				}

				s *= g; // re add the sign bit.

				// add "analog" which is just an even harmonic series, obtained by abs(s^3).
				// does not contain the original signal so add it.
				float analog = (std::abs(s * s * s)) * .25f; // scale it down feels more practical. note that this will be non-negative so a DC filter is required to pull it down.

				analog = mDC[chanIndex].ProcessSample(analog);
				analog = shape_sinclip(analog); // an attempt to keep the analog signal under control.
				s += mEvenHarmonicsGainLin * analog;

				s *= mAutoDriveCompensation;

				s *= mCompensationGain;
				return s;
			}

			void ProcessSample(float s0, float s1) {
				if (!mEnable) {
					output0 = s0;
					output1 = s1;
					return;
				}

				if (mPanMode == PanMode::MidSide) {
					M7::MSEncode(s0, s1, &s0, &s1);
				}

				float dry0 = s0;
				float dry1 = s1;

				float this0 = distort(s0, 0);
				float this1 = distort(s1, 1);

				s0 = this0;
				s1 = this1;

				// DO PANNING
				// do NOT use a pan law here; when it's centered it means both channels should get 100% wetness.
				float dryWet0 = 1;
				float dryWet1 = 1;
				if (mPanN11 < 0)dryWet1 = mPanN11 + 1.0f; // left chan (right attenuated)
				if (mPanN11 > 0) dryWet0 = 1.0f - mPanN11; // right chan (left attenuated)

				dryWet0 *= mDryWet;
				dryWet1 *= mDryWet;

				// note: when (e.g.) saturating only side channel, you'll get a signal that's too wide because of the natural
				// gain that saturation results in.
				// in these cases, dry/wet is very important param for balancing the stereo image again, in MidSide mode.
				s0 = M7::math::lerp(dry0, s0, dryWet0);
				s1 = M7::math::lerp(dry1, s1, dryWet1);

				if (mPanMode == PanMode::MidSide) {
					M7::MSDecode(s0, s1, &s0, &s1);
				}

				output0 = s0;
				output1 = s1;
			}
		}; // struct FreqBand



		float mParamCache[(int)ParamIndices::NumParams];

		M7::ParamAccessor mParams;

		Maj7Sat() :
			Device((int)ParamIndices::NumParams),
			mParams(mParamCache, 0)
		{
			LoadDefaults();
		}

		virtual void LoadDefaults() override
		{
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
			SetParam(0, mParamCache[0]); // force recalcing
		}

		static constexpr size_t gBandCount = 3;
		FreqBand mBands[gBandCount] = {
			{mParamCache, ParamIndices::AEnable },
			{mParamCache, ParamIndices::BEnable },
			{mParamCache, ParamIndices::CEnable },
		};

		float mInputGainLin = 0;
		float mOutputGainLin = 0;

		float mCrossoverFreqA = 0;
		float mCrossoverFreqB = 0;

		Oversampling mOversampling = Oversampling::Off;
		OutputSignal mOutputSignal = OutputSignal::WetCombined;

		M7::LinkwitzRileyFilter::Slope mCrossoverSlopeA = M7::LinkwitzRileyFilter::Slope::Slope_12dB;
		M7::LinkwitzRileyFilter::Slope mCrossoverSlopeB = M7::LinkwitzRileyFilter::Slope::Slope_12dB;

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;

			for (auto& b : mBands) {
				b.Slider();
			}

			mInputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
			mOutputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);

			mOutputSignal = mParams.GetEnumValue<OutputSignal>(ParamIndices::OutputSignal);

			mCrossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, -1, M7::gFilterFreqConfig, 0, 0);
			mCrossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, -1, M7::gFilterFreqConfig, 0, 0);

			mCrossoverSlopeA = mParams.GetEnumValue<M7::LinkwitzRileyFilter::Slope>(ParamIndices::CrossoverASlope);

			mOversampling = mParams.GetEnumValue<Oversampling>(ParamIndices::Oversampling);
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		M7::FrequencySplitter splitter0;
		M7::FrequencySplitter splitter1;

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				float s0 = inputs[0][i] * mInputGainLin;
				float s1 = inputs[1][i] * mInputGainLin;

				// split into 3 bands
				splitter0.frequency_splitter(s0, mCrossoverFreqA, mCrossoverSlopeA, mCrossoverFreqB);
				splitter1.frequency_splitter(s1, mCrossoverFreqA, mCrossoverSlopeA, mCrossoverFreqB);

				for (int iBand = 0; iBand < gBandCount; ++iBand) {
					auto& band = mBands[iBand];
					band.ProcessSample(splitter0.s[iBand], splitter1.s[iBand]);
				}

				switch (mOutputSignal) {
				case OutputSignal::DryBandLow:
					s0 = splitter0.s[0];
					s1 = splitter1.s[0];
					break;
				case OutputSignal::DryBandMid:
					s0 = splitter0.s[1];
					s1 = splitter1.s[1];
					break;
				case OutputSignal::DryBandHigh:
					s0 = splitter0.s[2];
					s1 = splitter1.s[2];
					break;
				case OutputSignal::DryCombined:
					s0 = splitter0.s[0] + splitter0.s[1] + splitter0.s[2];
					s1 = splitter1.s[0] + splitter1.s[1] + splitter1.s[2];
					break;
				case OutputSignal::WetBandLow:
					s0 = mBands[0].output0;
					s1 = mBands[0].output1;
					break;
				case OutputSignal::WetBandMid:
					s0 = mBands[1].output0;
					s1 = mBands[1].output1;
					break;
				case OutputSignal::WetBandHigh:
					s0 = mBands[2].output0;
					s1 = mBands[2].output1;
					break;
				case OutputSignal::WetCombined:
					s0 = mBands[0].output0 + mBands[1].output0 + mBands[2].output0;
					s1 = mBands[0].output1 + mBands[1].output1 + mBands[2].output1;
					break;
				}

				outputs[0][i] = s0 * mOutputGainLin;
				outputs[1][i] = s1 * mOutputGainLin;


			} // for i < numSamples
		} // Run()

	};
}
