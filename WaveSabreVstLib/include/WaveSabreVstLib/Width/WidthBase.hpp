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


inline ImU32 GetCorrellationColor(float correllation, float alpha) {
	if (correllation > 0.5f) {
		return ColorFromHTML("#66FF66", alpha); // Green for good correlation
	}
	else if (correllation < -0.5f) {
		return ColorFromHTML("#FF6666", alpha); // Red for bad correlation
	}
	else {
		return ColorFromHTML("#FFFF66", alpha); // Yellow for neutral
	}
}
