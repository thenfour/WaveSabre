
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

		static constexpr float gAnalogMaxLin = 2;

		enum class PanMode : uint8_t {
			Stereo,
			MidSide,
			Count__,
		};

		//enum class Oversampling : uint8_t {
		//	Off,
		//	x2,
		//	x4,
		//	x8,
		//	Count__,
		//};

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
			// note: enable/disable feels related to this param; it basically does a bypass.
			None, // mute this band.
			Diff, // output a diff / delta signal of sorts.
			Wet, // normal; output the processed signal.
			Count__,
		};

		//enum class EvenHarmonicsStyle : uint8_t {
		//	SqrtHard,
		//	SqrtSin,
		//	LinHard,
		//	LinSin,
		//	Count__,
		//};

		enum class ParamIndices
		{
			InputGain,
			CrossoverAFrequency,
			CrossoverASlope,
			CrossoverBFrequency,
			OutputGain,

			// low band
			AEnable,
			APanMode,
			APan,
			AModel,
			ADrive,
			ACompensationGain,
			AOutputGain,
			AThreshold,
			AEvenHarmonics,
			ADryWet,
			AOutputSignal,

			// mid band
			BEnable,
			BPanMode,
			BPan,
			BModel,
			BDrive,
			BCompensationGain,
			BOutputGain,
			BThreshold,
			BEvenHarmonics,
			BDryWet,
			BOutputSignal,

			// hi band
			CEnable,
			CPanMode,
			CPan,
			CModel,
			CDrive,
			CCompensationGain,
			COutputGain,
			CThreshold,
			CEvenHarmonics,
			CDryWet,
			COutputSignal,

			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7SAT_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Sat::ParamIndices::NumParams]{ \
			{"InpGain"}, /* InputGain */ \
			{"xAFreq"}, /* CrossoverAFrequency */ \
			{"xASlope"}, /* CrossoverASlope */ \
			{"xBFreq"}, /* CrossoverBFrequency */ \
			{"OutpGain"}, /* OutputGain */ \
		{"0Enable"}, /* enable */ \
		{"0PanMode"}, /* PanMode */ \
		{"0Pan"}, /* Pan */ \
		{"0Model"}, /* Model */ \
		{"0Drive"}, /* Drive */ \
		{"0CmpGain"}, /* CompensationGain */ \
		{"0OutGain"}, /* CompensationGain */ \
		{"0Thresh"}, /* Threshold */ \
		{"0Analog"}, /* EvenHarmonics */ \
		{"0DryWet"}, /* DryWet */ \
		{"0OutSig"}, /* output signal */ \
		{"1Enable"}, /* enable */ \
		{"1PanMode"}, /* PanMode */ \
		{"1Pan"}, /* Pan */ \
		{"1Model"}, /* Model */ \
		{"1Drive"}, /* Drive */ \
		{"1CmpGain"}, /* CompensationGain */ \
		{"1OutGain"}, /* CompensationGain */ \
		{"1Thresh"}, /* Threshold */ \
		{"1Analog"}, /* EvenHarmonics */ \
		{"1DryWet"}, /* DryWet */ \
		{"1OutSig"}, /* output signal */ \
		{"2Enable"}, /* enable */ \
		{"2PanMode"}, /* PanMode */ \
		{"2Pan"}, /* Pan */ \
		{"2Model"}, /* Model */ \
		{"2Drive"}, /* Drive */ \
		{"2CmpGain"}, /* CompensationGain */ \
		{"2OutGain"}, /* CompensationGain */ \
		{"2Thresh"}, /* Threshold */ \
		{"2Analog"}, /* EvenHarmonics */ \
		{"2DryWet"}, /* DryWet */ \
		{"2OutSig"}, /* output signal */ \
}


		static_assert((int)ParamIndices::NumParams == 38, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[38] = {
		  8230, // InpGain = 0.25118863582611083984
		  12052, // xAFreq = 0.3677978515625
		  40, // xASlope = 0.001220703125
		  20715, // xBFreq = 0.63219285011291503906
		  8230, // OutpGain = 0.25118863582611083984
		  0, // 0Enable = 0
		  -32768, // 0PanMode = -1
		  16384, // 0Pan = 0.5
		  24, // 0Model = 0.000732421875
		  4125, // 0Drive = 0.125885009765625
		  16422, // 0CmpGain = 0.50118720531463623047
		  16422, // 0OutGain = 0.50115966796875
		  20675, // 0Thresh = 0.63095736503601074219
		  1966, // 0Analog = 0.059999998658895492554
		  32767, // 0DryWet = 1
		  40, // 0OutSig = 0.001235177856869995594
		  32767, // 1Enable = 1
		  -32768, // 1PanMode = -1
		  16384, // 1Pan = 0.5
		  24, // 1Model = 0.000732421875
		  4125, // 1Drive = 0.125885009765625
		  16422, // 1CmpGain = 0.50115966796875
		  16422, // 1OutGain = 0.50115966796875
		  20675, // 1Thresh = 0.630950927734375
		  1966, // 1Analog = 0.059999998658895492554
		  32767, // 1DryWet = 0.999969482421875
		  40, // 1OutSig = 0.001235177856869995594
		  0, // 2Enable = 0
		  8, // 2PanMode = 0.000244140625
		  16384, // 2Pan = 0.5
		  24, // 2Model = 0.000732421875
		  4125, // 2Drive = 0.12589254975318908691
		  16422, // 2CmpGain = 0.50115966796875
		  16422, // 2OutGain = 0.50115966796875
		  20675, // 2Thresh = 0.630950927734375
		  1966, // 2Analog = 0.059999998658895492554
		  32767, // 2DryWet = 0.999969482421875
		  40, // 2OutSig = 0.001235177856869995594
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
				OutputGain,
				Threshold,
				EvenHarmonics,
				DryWet,
				OutputSignal,
			};

			float mDriveLin ;
			float mCompensationGain ;
			float mOutputGain ;
			float mAutoDriveCompensation ;
			float mThresholdLin ;
			float mDryWet ;
			float mEvenHarmonicsGainLin ;
			float mPanN11 ;
			float mCorrSlope;

			float output0 ;
			float output1 ;
			float diff0 ;
			float diff1;

			Model mModel;
			PanMode mPanMode;
			OutputSignal mOutputSignal;
			bool mEnable = false;

			M7::ParamAccessor mParams;
			M7::DCFilter mDC[2];

			FreqBand(float* paramCache, ParamIndices baseParamID) : //
				mParams(paramCache, baseParamID)
			{}

			void Slider() {

				mEnable = mParams.GetBoolValue(BandParam::Enable);
				mDriveLin = mParams.GetLinearVolume(BandParam::Drive, M7::gVolumeCfg36db);
				mCompensationGain = mParams.GetLinearVolume(BandParam::CompensationGain, M7::gVolumeCfg12db);
				mOutputGain = mParams.GetLinearVolume(BandParam::OutputGain, M7::gVolumeCfg12db);
				mModel = mParams.GetEnumValue<Model>(BandParam::Model);

				mThresholdLin = mParams.GetLinearVolume(BandParam::Threshold, M7::gUnityVolumeCfg);
				// threshold must be corrected, otherwise it will cause a div by 0 when we scale the waveform up.
				mThresholdLin = M7::math::clamp(mThresholdLin, 0, 0.99f);

				mDryWet = mParams.Get01Value(BandParam::DryWet, 0);
				mEvenHarmonicsGainLin = mParams.GetScaledRealValue(BandParam::EvenHarmonics, 0, gAnalogMaxLin, 0); //mParams.GetLinearVolume(BandParam::EvenHarmonics, M7::gVolumeCfg36db, 0);

				mPanMode = mParams.GetEnumValue<PanMode>(BandParam::PanMode);
				mPanN11 = mParams.GetN11Value(BandParam::Pan, 0);
				
				mOutputSignal = mParams.GetEnumValue<OutputSignal>(BandParam::OutputSignal);

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

			// hard-clipping at 1 doesn't make sense because it renders threshold pointless.
			// hard-clipping at 0.5 does a good job of keeping loudness the same and also giving meaning to threshold.
			static float shape_hardclip(float in)
			{
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
			static float shape_trifold(float s)
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

			// performs the saturation, used for VST GUI as well.
			float transfer(float s, float& diffSignal) const {
				// calculating a diff signal has some quirks. if you just do wet-dry, you'll find that it's not so helpful,
				// mostly because we do a lot of gain application.
				// The dry signal is just before this function is called. so keep track of all these gain applications so we can undo it for a better diff.

				float g = s < 0 ? -1.0f : 1.0f;
				s = std::abs(s); // work only in positive pole for shaping.

				// at thresh=1, no shaping occurs and slope is 1 (linear).
				// at thresh=0, the whole curve is shaped so gets the natural slope of the model = pi/2.
				s *= mCorrSlope;

				if (s > mThresholdLin) {
					// we will be shaping the area above thresh, so map (thresh,1) to (0,1)
					float s_preshape = s;

					s -= mThresholdLin;
					s /= (1.0f - mThresholdLin);
					s /= ModelNaturalSlopes[(int)mModel];

					// perform shaping
					switch (mModel) {
					default:
					case Model::Thru:
						break;// nop
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
					diffSignal = (s - s_preshape) * g;
				}
				else {
					diffSignal = 0;
				}

				s *= g; // re add the sign bit.
				return s;
			}

			float distort(float s, size_t chanIndex, float& diffSignal)
			{
				s = transfer(s, diffSignal);
				float analog = s * s - .5f;
				analog = mDC[chanIndex].ProcessSample(analog);
				analog = M7::math::clamp01(analog);// must clip, otherwise values can be huge if s happened to be > 1
				analog *= mEvenHarmonicsGainLin;
				s += analog;
				diffSignal += analog;

				s *= mAutoDriveCompensation;
				s *= mCompensationGain;

				diffSignal /= std::max(1.0f, mDriveLin);

				return s;
			}

			void ProcessSample(float s0, float s1) {
				if (mEnable) {
					if (mPanMode == PanMode::MidSide) {
						M7::MSEncode(s0, s1, &s0, &s1);
					}

					float dry0 = s0;
					float dry1 = s1;

					s0 *= ModelPregain[(int)mModel];
					s0 *= mDriveLin;
					s1 *= ModelPregain[(int)mModel];
					s1 *= mDriveLin;

					float this0 = distort(s0, 0, diff0);
					float this1 = distort(s1, 1, diff1);

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
						M7::MSDecode(diff0, diff1, &diff0, &diff1);
					}
					diff0 = M7::math::clampN11(diff0);
					diff1 = M7::math::clampN11(diff1);
				}
				else {
					diff0 = 0;
					diff1 = 0;
				}

				switch (mOutputSignal) {
				case OutputSignal::None:
					output0 = 0;
					output1 = 0;
					break;
				case OutputSignal::Diff:
					output0 = diff0;
					output1 = diff1;
					break;
				case OutputSignal::Wet:
					output0 = s0;
					output1 = s1;
					break;
				}
				output0 *=  mOutputGain;
				output1 *=  mOutputGain;

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

			mCrossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, -1, M7::gFilterFreqConfig, 0, 0);
			mCrossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, -1, M7::gFilterFreqConfig, 0, 0);

			mCrossoverSlopeA = mParams.GetEnumValue<M7::LinkwitzRileyFilter::Slope>(ParamIndices::CrossoverASlope);
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

				s0 = mBands[0].output0 + mBands[1].output0 + mBands[2].output0;
				s1 = mBands[0].output1 + mBands[1].output1 + mBands[2].output1;

				//s0 = splitter0.s[0] + splitter0.s[1] + splitter0.s[2];
				//s1 = splitter1.s[0] + splitter1.s[1]+ splitter1.s[2];

				// for sanity; avoid user error or filter madness causing crazy spikes.
				// so clamp at about +12db clip. todo: param smoothing to avoid these spikes.
				outputs[0][i] = M7::math::clamp(s0 * mOutputGainLin, -4, 4);
				outputs[1][i] = M7::math::clamp(s1 * mOutputGainLin, -4, 4);



			} // for i < numSamples
		} // Run()

	};
}
