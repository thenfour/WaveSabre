#pragma once

// a param reader that smooths its output value over time.

#include "../Basic/DSPMath.hpp"

namespace WaveSabreCore::M7
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Deterministic integer hash -> [-1, +1]
float hash1D(int n);
// 2D lattice hash -> [-1, +1]
float hash2D(int x, int y);

// Quintic fade curve
// 6t^5 - 15t^4 + 10t^3
// float fade5(float t);
// float fade3(float t);

// -1..1 output
float valueNoise2Df(float x, float y);
double valueNoise2D(double x, double y);

//float valueNoise1D(float x);

float fbm2Df(const FloatPair& p);
double fbm2D(const DoublePair& p);

} // namespace WaveSabreCore::M7
