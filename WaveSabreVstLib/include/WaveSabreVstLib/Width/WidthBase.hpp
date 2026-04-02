#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

//#include "Maj7WidthVst.h"
#include <map>
#include <array>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>


struct StereoImagingColorScheme {
	ImU32 okColor = ColorFromHTML("#66FF66");
	ImU32 warningColor = ColorFromHTML("#FFFF66");
	ImU32 alertColor = ColorFromHTML("#FF6666");
};

inline const StereoImagingColorScheme& GetDefaultStereoImagingColorScheme() {
	static const StereoImagingColorScheme scheme{};
	return scheme;
}

inline ImU32 ApplyAlphaToColor(ImU32 color, float alpha) {
	alpha = std::clamp(alpha, 0.0f, 1.0f);
	return (color & 0x00FFFFFFu) | (static_cast<ImU32>(alpha * 255.0f) << IM_COL32_A_SHIFT);
}



inline ImU32 GetCorrellationColor(float correllation,
		float alpha,
		const StereoImagingColorScheme& colorScheme = GetDefaultStereoImagingColorScheme()) {
	if (correllation > 0.5f) {
		return ApplyAlphaToColor(colorScheme.okColor, alpha);
	}
	else if (correllation < -0.5f) {
		return ApplyAlphaToColor(colorScheme.alertColor, alpha);
	}
	else {
		return ApplyAlphaToColor(colorScheme.warningColor, alpha);
	}
}
