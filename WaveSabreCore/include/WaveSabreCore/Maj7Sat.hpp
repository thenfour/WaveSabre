
#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
#include <vector>
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

//#define MAJ7SAT_ENABLE_RARE_MODELS
//#define MAJ7SAT_ENABLE_ANALOG

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
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
			DivClipSoft,
#endif // MAJ7SAT_ENABLE_RARE_MODELS
			TanhClip,
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
			DivClipMedium,
			DivClipHard,
			HardClip,
			TanhFold,
			TanhSinFold,
			SinFold,
			LinearFold,
#endif // MAJ7SAT_ENABLE_RARE_MODELS
			Count__,
		};

#ifdef MAJ7SAT_ENABLE_RARE_MODELS

#define MAJ7SAT_MODEL_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Sat::Model::Count__] { \
			"Thru", \
		"SineClip", \
		"DivClipSoft", \
			"TanhClip", \
			"DivClipMedium", \
			"DivClipHard", \
			"HardClip", \
			"TanhFold", \
			"TanhSinFold", \
			"SinFold", \
			"LinearFold", \
#endif // MAJ7SAT_ENABLE_RARE_MODELS
		}
#else // MAJ7SAT_ENABLE_RARE_MODELS
#define MAJ7SAT_MODEL_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Sat::Model::Count__] { \
			"Thru", \
		"SineClip", \
			"TanhClip", \
		}


#endif // MAJ7SAT_ENABLE_RARE_MODELS


		// helps unify the sound of them. nothing magic here just values that seemed to work.
		// the method is to just keep a fundamental frequency constant, and let the harmonics add.
		// so using a fft meter to match a sine wave fundamental level.
		static constexpr float ModelPregain[(size_t)Model::Count__] = {
			1, // thru,
			0.71f, // sinclip
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
			0.69f, // div 41
#endif // MAJ7SAT_ENABLE_RARE_MODELS
			0.62f, // tanh clip
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
			0.47f, // DivClipMedium,
			0.21f, // DivClipHard,
			1, // HardClip,
			0.41f,// TanhFold,
			0.56f,// TanhSinFold,
			0.71f,// SinFold,
			1, // LinearFold,
#endif // MAJ7SAT_ENABLE_RARE_MODELS
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
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
			gK41 + 1, // div 41
#endif // MAJ7SAT_ENABLE_RARE_MODELS
			2, // tanh clip
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
			gK63 + 1, // DivClipMedium,
			gK85 + 1, // DivClipHard,
			1, // HardClip,
			M7::math::gPI,// TanhFold, <-- really?
			M7::math::gPI * 3.0f / 4.0f,// TanhSinFold,
			M7::math::gPIHalf,// SinFold,
			1, // LinearFold,
#endif // MAJ7SAT_ENABLE_RARE_MODELS
		};

		//enum class OutputSignal : uint8_t {
		//	Normal, // process this band (wet out)
		//	Bypass, // don't process this band (dry out)
		//	Diff, // output a diff / delta signal of sorts.
		//	Count__,
		//};

		enum class ParamIndices
		{
			InputGain,
			CrossoverAFrequency,
			CrossoverASlope,
			CrossoverBFrequency,
			OverallDryWet,
			OutputGain,

			// low band
			AMute,
			ASolo,
			APanMode,
			APan,
			AModel,
			ADrive,
			ACompensationGain,
			AOutputGain,
			AThreshold,
			AEvenHarmonics,
			ADryWet,
			AEnable,

			// mid band
			BMute,
			BSolo,
			BPanMode,
			BPan,
			BModel,
			BDrive,
			BCompensationGain,
			BOutputGain,
			BThreshold,
			BEvenHarmonics,
			BDryWet,
			BEnable,

			// hi band
			CMute,
			CSolo,
			CPanMode,
			CPan,
			CModel,
			CDrive,
			CCompensationGain,
			COutputGain,
			CThreshold,
			CEvenHarmonics,
			CDryWet,
			CEnable,

			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7SAT_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Sat::ParamIndices::NumParams]{ \
			{"InpGain"}, /* InputGain */ \
			{"xAFreq"}, /* CrossoverAFrequency */ \
			{"xASlope"}, /* CrossoverASlope */ \
			{"xBFreq"}, /* CrossoverBFrequency */ \
			{"GDryWet"}, /* drywet global */ \
			{"OutpGain"}, /* OutputGain */ \
		{"0Mute"}, \
		{"0Solo"}, \
		{"0PanMode"}, /* PanMode */ \
		{"0Pan"}, /* Pan */ \
		{"0Model"}, /* Model */ \
		{"0Drive"}, /* Drive */ \
		{"0CmpGain"}, /* CompensationGain */ \
		{"0OutGain"}, /* CompensationGain */ \
		{"0Thresh"}, /* Threshold */ \
		{"0Analog"}, /* EvenHarmonics */ \
		{"0DryWet"}, /* DryWet */ \
		{"0Enable"}, /* */\
		{"1Mute"}, \
		{"1Solo"}, \
		{"1PanMode"}, /* PanMode */ \
		{"1Pan"}, /* Pan */ \
		{"1Model"}, /* Model */ \
		{"1Drive"}, /* Drive */ \
		{"1CmpGain"}, /* CompensationGain */ \
		{"1OutGain"}, /* CompensationGain */ \
		{"1Thresh"}, /* Threshold */ \
		{"1Analog"}, /* EvenHarmonics */ \
		{"1DryWet"}, /* DryWet */ \
		{"1Enable"}, /* */\
		{"2Mute"}, \
		{"2Solo"}, \
		{"2PanMode"}, /* PanMode */ \
		{"2Pan"}, /* Pan */ \
		{"2Model"}, /* Model */ \
		{"2Drive"}, /* Drive */ \
		{"2CmpGain"}, /* CompensationGain */ \
		{"2OutGain"}, /* CompensationGain */ \
		{"2Thresh"}, /* Threshold */ \
		{"2Analog"}, /* EvenHarmonics */ \
		{"2DryWet"}, /* DryWet */ \
		{"2Enable"}, /* */\
}

		static_assert((int)ParamIndices::NumParams == 42, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[42] = {
		  8230, // InpGain = 0.25115966796875
		  13557, // xAFreq = 0.4137503504753112793
		  40, // xASlope = 0.001220703125
		  21577, // xBFreq = 0.65849626064300537109
		  32767, // GDryWet = 0.999969482421875
		  8230, // OutpGain = 0.25115966796875
		  0, // 0Mute = 0
		  0, // 0Solo = 0
		  -32768, // 0PanMode = -1
		  16384, // 0Pan = 0.5
		  24, // 0Model = 0.000732421875
		  4125, // 0Drive = 0.125885009765625
		  16422, // 0CmpGain = 0.50115966796875
		  16422, // 0OutGain = 0.50115966796875
		  20675, // 0Thresh = 0.630950927734375
		  1966, // 0Analog = 0.05999755859375
		  32767, // 0DryWet = 0.999969482421875
		  24, // 0OutSig = 0.00074110669083893299103
		  0, // 1Mute = 0
		  0, // 1Solo = 0
		  -32768, // 1PanMode = -1
		  16384, // 1Pan = 0.5
		  24, // 1Model = 0.000732421875
		  4125, // 1Drive = 0.125885009765625
		  16422, // 1CmpGain = 0.50115966796875
		  16422, // 1OutGain = 0.50115966796875
		  20675, // 1Thresh = 0.630950927734375
		  1966, // 1Analog = 0.05999755859375
		  32767, // 1DryWet = 0.999969482421875
		  8, // 1OutSig = 0.0002470355830155313015
		  0, // 2Mute = 0
		  0, // 2Solo = 0
		  8, // 2PanMode = 0.000244140625
		  16384, // 2Pan = 0.5
		  24, // 2Model = 0.000732421875
		  4125, // 2Drive = 0.125885009765625
		  16422, // 2CmpGain = 0.50115966796875
		  16422, // 2OutGain = 0.50115966796875
		  20675, // 2Thresh = 0.630950927734375
		  1966, // 2Analog = 0.05999755859375
		  32767, // 2DryWet = 0.999969482421875
		  24, // 2OutSig = 0.00074110669083893299103
		};

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct FreqBand {
			enum class BandParam {
				Mute,
				Solo,
				PanMode,
				Pan,
				Model,
				Drive,
				CompensationGain,
				OutputGain,
				Threshold,
				EvenHarmonics,
				DryWet,
				EnableEffect,
			};

			float mDriveLin ;
			float mCompensationGain ;
			float mOutputGain ;
			float mAutoDriveCompensation ;
			float mThresholdLin ;
			float mDryWet ;
#ifdef MAJ7SAT_ENABLE_ANALOG
			float mEvenHarmonicsGainLin ;
#endif // MAJ7SAT_ENABLE_ANALOG
			float mPanN11 ;
			float mCorrSlope;

			//float outputs[2];

			Model mModel;
			PanMode mPanMode;
			bool mEnableEffect;
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			bool mMute;
			bool mSolo;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			M7::ParamAccessor mParams;
#ifdef MAJ7SAT_ENABLE_ANALOG
			M7::DCFilter mDC[2];
#endif // MAJ7SAT_ENABLE_ANALOG

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			AnalysisStream mInputAnalysis0;
			AnalysisStream mInputAnalysis1;
			AnalysisStream mOutputAnalysis0;
			AnalysisStream mOutputAnalysis1;

			bool mMuteSoloEnabled;
#else
			static constexpr bool mMuteSoloEnabled = true;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			FreqBand(float* paramCache, ParamIndices baseParamID) : //
				mParams(paramCache, baseParamID)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				,
				mInputAnalysis0(700),
				mInputAnalysis1(700),
				mOutputAnalysis0(700),
				mOutputAnalysis1(700)
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
			{}

			void Slider() {

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mMute = mParams.GetBoolValue(BandParam::Mute);
				mSolo = mParams.GetBoolValue(BandParam::Solo);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
				mEnableEffect = mParams.GetBoolValue(BandParam::EnableEffect);

				mDriveLin = mParams.GetLinearVolume(BandParam::Drive, M7::gVolumeCfg36db);
				mCompensationGain = mParams.GetLinearVolume(BandParam::CompensationGain, M7::gVolumeCfg12db);
				mOutputGain = mParams.GetLinearVolume(BandParam::OutputGain, M7::gVolumeCfg12db);
				mThresholdLin = mParams.GetLinearVolume(BandParam::Threshold, M7::gUnityVolumeCfg);
				// threshold must be corrected, otherwise it will cause a div by 0 when we scale the waveform up.
				mThresholdLin = M7::math::clamp(mThresholdLin, 0, 0.99f);

				mModel = mParams.GetEnumValue<Model>(BandParam::Model);

				mDryWet = mParams.Get01Value(BandParam::DryWet);
#ifdef MAJ7SAT_ENABLE_ANALOG
				mEvenHarmonicsGainLin = mParams.GetScaledRealValue(BandParam::EvenHarmonics, 0, gAnalogMaxLin, 0); //mParams.GetLinearVolume(BandParam::EvenHarmonics, M7::gVolumeCfg36db, 0);
#endif // MAJ7SAT_ENABLE_ANALOG

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
			float transfer(float s) const {
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

					s -= mThresholdLin;
					s /= (1.0f - mThresholdLin);
					s /= ModelNaturalSlopes[(int)mModel];

					// perform shaping
					//float s_preshape = s;
					switch (mModel) {
					default:
					case Model::Thru:
						break;// nop
					case Model::SineClip:
						s = shape_sinclip(s);
						break;
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
					case Model::DivClipSoft:
						s = shape_div(s, gK41);
						break;
#endif // MAJ7SAT_ENABLE_RARE_MODELS
					case Model::TanhClip:
						s = shape_tanhclip(s);
						break;
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
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
#endif // MAJ7SAT_ENABLE_RARE_MODELS
					}
					//satAmount = s - s_preshape;

					// now map back (0,1) to (thresh,1).
					s *= (1.0f - mThresholdLin);
					s += mThresholdLin;
					//diffSignal = (s - s_preshape) * g;
				}
				//else {
				//	diffSignal = 0;
				//}

				s *= g; // re add the sign bit.
				return s;
			}

			float distort(float s, size_t chanIndex)
			{
				s = transfer(s);
#ifdef MAJ7SAT_ENABLE_ANALOG
				float analog = s * s - .5f;
				analog = mDC[chanIndex].ProcessSample(analog);
				analog = M7::math::clamp01(analog);// must clip, otherwise values can be huge if s happened to be > 1
				analog *= mEvenHarmonicsGainLin;
				s += analog;
#endif // MAJ7SAT_ENABLE_ANALOG

				s *= mAutoDriveCompensation;
				s *= mCompensationGain;

				return s;
			}

			M7::FloatPair ProcessSample(float s0, float s1, float masterDryWet)
			{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (mEnableEffect && mMuteSoloEnabled) {
					mInputAnalysis0.WriteSample(s0);
					mInputAnalysis1.WriteSample(s1);
				}
				else {
					// analysis halts when not processing. it's better visually.
					mInputAnalysis0.WriteSample(0);
					mInputAnalysis1.WriteSample(0);
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				float sa[2] = { s0, s1 };
				float dry[2] = { s0, s1 };
				//float dry0 = s0;
				//float dry1 = s1;

				if (mEnableEffect && mMuteSoloEnabled) {
					if (mPanMode == PanMode::MidSide) {
						M7::MSEncode(sa[0], sa[1], &sa[0], &sa[1]);
						dry[0] = sa[0];
						dry[1] = sa[1];
					}

					// DO PANNING
					// do NOT use a pan law here; when it's centered it means both channels should get 100% wetness.
					float dryWet[2] = { 1,1 };
					if (mPanN11 < 0) dryWet[1] = mPanN11 + 1.0f; // left chan (right attenuated)
					if (mPanN11 > 0) dryWet[0] = 1.0f - mPanN11; // right chan (left attenuated)

					for (int i = 0; i < 2; ++i) {
						float& s = *(sa + i);

						s *= ModelPregain[(int)mModel];
						s *= mDriveLin;
						s = distort(s, i);

						// note: when (e.g.) saturating only side channel, you'll get a signal that's too wide because of the natural
						// gain that saturation results in.
						// in these cases, dry/wet is very important param for balancing the stereo image again, in MidSide mode.
						s = M7::math::lerp(dry[i], s, dryWet[i] * mDryWet * masterDryWet);
					}

					//s0 *= ModelPregain[(int)mModel];
					//s1 *= ModelPregain[(int)mModel];

					//s0 *= mDriveLin;
					//s1 *= mDriveLin;

					//float this0 = distort(s0, 0);
					//float this1 = distort(s1, 1);

					//s0 = this0;
					//s1 = this1;

					//// DO PANNING
					//// do NOT use a pan law here; when it's centered it means both channels should get 100% wetness.
					//float dryWet0 = 1;
					//float dryWet1 = 1;
					//if (mPanN11 < 0) dryWet1 = mPanN11 + 1.0f; // left chan (right attenuated)
					//if (mPanN11 > 0) dryWet0 = 1.0f - mPanN11; // right chan (left attenuated)

					//dryWet0 *= mDryWet * masterDryWet;
					//dryWet1 *= mDryWet * masterDryWet;

					//// note: when (e.g.) saturating only side channel, you'll get a signal that's too wide because of the natural
					//// gain that saturation results in.
					//// in these cases, dry/wet is very important param for balancing the stereo image again, in MidSide mode.
					//s0 = M7::math::lerp(dry0, s0, dryWet0);
					//s1 = M7::math::lerp(dry1, s1, dryWet1);

					if (mPanMode == PanMode::MidSide) {
						M7::MSDecode(sa[0], sa[1], &sa[0], &sa[1]);
					}
					//diff0 = M7::math::clampN11(diff0);
					//diff1 = M7::math::clampN11(diff1);
				}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				float output0, output1;
				if (mMuteSoloEnabled) {
					if (mEnableEffect) {
						output0 = sa[0];
						output1 = sa[1];
					}
					else {
						output0 = dry[0];
						output1 = dry[1];
					}
					output0 *= mOutputGain;
					output1 *= mOutputGain;
				}
				else {
					output0 = 0;
					output1 = 0;
				}

				if (mEnableEffect && mMuteSoloEnabled) {
					mOutputAnalysis0.WriteSample(output0);
					mOutputAnalysis1.WriteSample(output1);
				}
				else {
					// analysis halts when not processing. it's better visually.
					mOutputAnalysis0.WriteSample(0);
					mOutputAnalysis1.WriteSample(0);
				}
				return {output0, output1};
#else
				return {sa[0], sa[1]};
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			}
		}; // struct FreqBand

		float mParamCache[(int)ParamIndices::NumParams];

		M7::ParamAccessor mParams;

		Maj7Sat() :
			Device((int)ParamIndices::NumParams),
			mParams(mParamCache, 0)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			,
			mInputAnalysis0(1000),
			mInputAnalysis1(1000),
			mOutputAnalysis0(1000),
			mOutputAnalysis1(1000)
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
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
			{mParamCache, ParamIndices::AMute },
			{mParamCache, ParamIndices::BMute},
			{mParamCache, ParamIndices::CMute},
		};

		float mInputGainLin = 0;
		float mOutputGainLin = 0;

		float mCrossoverFreqA = 0;
		float mCrossoverFreqB = 0;

		M7::LinkwitzRileyFilter::Slope mCrossoverSlopeA = M7::LinkwitzRileyFilter::Slope::Slope_12dB;
		//M7::LinkwitzRileyFilter::Slope mCrossoverSlopeB = M7::LinkwitzRileyFilter::Slope::Slope_12dB;

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;

			for (auto& b : mBands) {
				b.Slider();
			}

			mInputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
			mOutputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);

			mCrossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, M7::gFilterFreqConfig);
			mCrossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, M7::gFilterFreqConfig);

			mCrossoverSlopeA = mParams.GetEnumValue<M7::LinkwitzRileyFilter::Slope>(ParamIndices::CrossoverASlope);
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		M7::FrequencySplitter splitter0;
		M7::FrequencySplitter splitter1;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis0;
		AnalysisStream mInputAnalysis1;
		AnalysisStream mOutputAnalysis0;
		AnalysisStream mOutputAnalysis1;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			// update "disable because mute or solo".
			bool soloExists = false;

			// Check if there's at least one Solo
			for (const auto& band : mBands) {
				if (band.mSolo) {
					soloExists = true;
					break;
				}
			}

			for (auto& band : mBands) {
				if (soloExists) {
					band.mMuteSoloEnabled = band.mSolo;
				}
				else {
					band.mMuteSoloEnabled = !band.mMute;
				}
			}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			float masterDryWet = mParams.GetRawVal(ParamIndices::OverallDryWet);

			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				float s0 = inputs[0][i] * mInputGainLin;
				float s1 = inputs[1][i] * mInputGainLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mInputAnalysis0.WriteSample(s0);
				mInputAnalysis1.WriteSample(s1);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				// split into 3 bands
				splitter0.frequency_splitter(s0, mCrossoverFreqA, mCrossoverSlopeA, mCrossoverFreqB);
				splitter1.frequency_splitter(s1, mCrossoverFreqA, mCrossoverSlopeA, mCrossoverFreqB);

				s0 = 0;
				s1 = 0;
				for (int iBand = 0; iBand < gBandCount; ++iBand) {
					auto& band = mBands[iBand];
					auto r = band.ProcessSample(splitter0.s[iBand], splitter1.s[iBand], masterDryWet);
					s0 += r.first * mOutputGainLin;
					s1 += r.second * mOutputGainLin;
				}

				//s0 = mBands[0].output0 + mBands[1].output0 + mBands[2].output0;
				//s1 = mBands[0].output1 + mBands[1].output1 + mBands[2].output1;

				//s0 *= mOutputGainLin;
				//s1 *= mOutputGainLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mOutputAnalysis0.WriteSample(s0);
				mOutputAnalysis1.WriteSample(s1);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				// for sanity; avoid user error or filter madness causing crazy spikes.
				// so clamp at about +12db clip. todo: param smoothing to avoid these spikes.
				outputs[0][i] = M7::math::clamp(s0, -4, 4);
				outputs[1][i] = M7::math::clamp(s1, -4, 4);
			} // for i < numSamples
		} // Run()

	};
}
