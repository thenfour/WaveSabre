#pragma once

#include "../GigaSynth/Maj7Basic.hpp"
#include "../Filters/DCFilter.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct Maj7SaturationBase
{
	// for the div style shaping, `k+1` also happens to represent the slope
	static constexpr float gK41 = 1.38983050847f;
	static constexpr float gK63 = 3.40540540541f;
	static constexpr float gK85 = 11.3333333333f;

#ifdef MAJ7SAT_ENABLE_RARE_MODELS
	enum class Model //: uint8_t
	{
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

	inline static constexpr char const* const ModelCaptions[(int)Model::Count__] = {
		"Thru",
		"SineClip",
		"DivClipSoft",
		"TanhClip",
		"DivClipMedium",
		"DivClipHard",
		"HardClip",
		"TanhFold",
		"TanhSinFold",
		"SinFold",
		"LinearFold",
	};

	inline static constexpr float ModelPregain[(int)Model::Count__] = {
		1.0f,
		0.71f,
		0.69f,
		0.62f,
		0.47f,
		0.21f,
		1.0f,
		0.41f,
		0.56f,
		0.71f,
		1.0f,
	};

	inline static constexpr float ModelNaturalSlopes[(int)Model::Count__] = {
		1.0f,
		M7::math::gPIHalf,
		gK41 + 1,
		2.0f,
		gK63 + 1,
		gK85 + 1,
		1.0f,
		M7::math::gPI,
		M7::math::gPI * 3.0f / 4.0f,
		M7::math::gPIHalf,
		1.0f,
	};
#else
	enum class Model //: uint8_t
	{
		Thru,
		TanhClip,
		DivClipHard,
		Count__,
	};

	inline static constexpr char const* const ModelCaptions[(int)Model::Count__] = {
		"Thru",
		"TanhClip",
		"DivClipHard",
	};

	inline static constexpr float ModelPregain[(int)Model::Count__] = {
		1.0f,
		0.62f,
		0.21f,
	};

	inline static constexpr float ModelNaturalSlopes[(int)Model::Count__] = {
		1.0f,
		2.0f,
		gK85 + 1,
	};
#endif

	// Patrice Tarrabia and Bram de Jong style waveshaper
	static float ShapeDiv(float sample, float k)
	{
		return (1.0f + k) * sample / (1.0f + k * sample);
	}

	static float ShapeSinClip(float s)
	{
		return M7::math::sin(M7::math::gPIHalf * M7::math::clamp01(s));
	}

	static float ShapeTanhClip(float in)
	{
		return M7::math::tanh(in * 2);
	}

	// hard-clipping at 1 doesn't make sense because it renders threshold pointless.
	static float ShapeHardClip(float in)
	{
		return std::min(in, 0.5f);
	}

	static float ShapeSinFold(float in)
	{
		return M7::math::sin(M7::math::gPIHalf * in);
	}

	static float ShapeTanhFold(float in)
	{
		return M7::math::tanh(2 * M7::math::sin(in * M7::math::gPIHalf));
	}

	static float ShapeTriFold(float s)
	{
		s = .25f * s - .25f;
		float b = M7::math::floor(s);
		s = std::abs(2 * (s - b - .5f));
		s = 2 * s - 1;
		return s;
	}

	static float ShapeTanhSinFold(float in)
	{
		return M7::math::tanh(1.5f * M7::math::sin(in * M7::math::gPIHalf));
	}

	static float CalcAutoDriveCompensation(float driveLin)
	{
		return (driveLin > 1.0f) ? (1.0f / driveLin) : 1.0f;
	}

	// Returns soft-clipped sample with optional transfer-gain metric used by some visualizers.
	// The reported gain is the instantaneous ratio between the clipped magnitude and the original
	// pre-makeup magnitude, so it reflects the full soft-knee region instead of only the hard-limited tail.
	static float SoftClipSine(float s, float thresh, float outputGain
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		, float* attenuationOut = nullptr
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
	)
	{
		thresh = M7::math::clamp(thresh, 0.01f, 0.99f);

		float sign = 1.0f;
		if (s < 0)
		{
			sign = -1.0f;
			s = -s;
		}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		const float inputMagnitude = std::fabs(s);
		float transferGain = 1.0f;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
		static constexpr float naturalSlope = M7::math::gPIHalf;
		//const float corrSlope = M7::math::lerp(naturalSlope, 1.0f, thresh);

		//s *= corrSlope;
		if (s > thresh)
		{
			s = M7::math::lerp_rev(thresh, 1.0f, s);
			s /= naturalSlope;
			if (s > 1.0f)
			{
				s = 1.0f;
			}
			else
			{
				s = ShapeSinClip(s);
			}
			s = M7::math::lerp(thresh, 1.0f, s);

		#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			if (inputMagnitude > 1.0e-12f)
			{
				transferGain = M7::math::clamp(s / inputMagnitude, 0.0f, 1.0f);
			}
		#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
		}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		if (attenuationOut)
		{
			*attenuationOut = transferGain;
		}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		return s * sign * outputGain;
	}

	static float Transfer(float s, Model model, float thresholdLin, float /*corrSlope*/)
	{
		float g = 1;
		if (s < 0)
		{
			g = -1;
			s = -s;
		}

		if (s > thresholdLin)
		{
			s = M7::math::map(s, thresholdLin, 1, 0, 1);
			s /= ModelNaturalSlopes[(int)model];

			switch (model)
			{
			case Model::Thru:
				break;
			default: // allow saturation even for unknown models; allows compat between versions when new models are added.
			case Model::TanhClip:
				s = ShapeTanhClip(s);
				break;
			case Model::DivClipHard:
				s = ShapeDiv(s, gK85);
				break;
#ifdef MAJ7SAT_ENABLE_RARE_MODELS
			case Model::SineClip:
				s = ShapeSinClip(s);
				break;
			case Model::DivClipSoft:
				s = ShapeDiv(s, gK41);
				break;
			case Model::HardClip:
				s = ShapeHardClip(s);
				break;
			case Model::TanhFold:
				s = ShapeTanhFold(s);
				break;
			case Model::TanhSinFold:
				s = ShapeTanhSinFold(s);
				break;
			case Model::SinFold:
				s = ShapeSinFold(s);
				break;
			case Model::LinearFold:
				s = ShapeTriFold(s);
				break;
			case Model::DivClipMedium:
				s = ShapeDiv(s, gK63);
				break;
#endif
			}

			s = M7::math::lerp(thresholdLin, 1, s);
		}

		s *= g;
		return s;
	}

#ifdef MAJ7SAT_ENABLE_ANALOG
	static float ApplyAnalog(float s, size_t chanIndex, M7::DCFilter* dcFilters, float evenHarmonicsGainLin)
	{
		if (!dcFilters)
			return s;

		float analog = s * s - .5f;
		analog = dcFilters[chanIndex].ProcessSample(analog);
		analog = M7::math::clamp01(analog);
		analog *= evenHarmonicsGainLin;
		s += analog;
		return s;
	}
#endif

	static float DistortSample(float s,
													 size_t chanIndex, // needed for analog part to know which DC filter to use
													 Model model,
													 float thresholdLin,
													 float corrSlope,
													 float autoDriveCompensation
#ifdef MAJ7SAT_ENABLE_ANALOG
													 , M7::DCFilter* dcFilters, // needed for analog part; one dc filter per channel
													 float evenHarmonicsGainLin
#endif
													 )
	{
		s = Transfer(s, model, thresholdLin, corrSlope);
#ifdef MAJ7SAT_ENABLE_ANALOG
		s = ApplyAnalog(s, chanIndex, dcFilters, evenHarmonicsGainLin);
#endif
		s *= autoDriveCompensation;
		return s;
	}
};
} // namespace M7
} // namespace WaveSabreCore

