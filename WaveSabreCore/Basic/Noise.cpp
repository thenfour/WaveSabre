
// a param reader that smooths its output value over time.

#include "./Noise.hpp"

namespace WaveSabreCore::M7
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Deterministic integer hash -> [-1, +1]
float hash1D(int n)
{
  n = (n << 13) ^ n;
  const int nn = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
  return 1.0f - (float(nn) / 1073741824.0f);  // [-1, +1]
}

// 2D lattice hash -> [-1, +1]
float hash2D(int x, int y)
{
  // Mix coordinates into one integer before hashing.
  // Large odd constants help decorrelate axes.
  const int n = x * 1619 + y * 31337 + 1013;
  return hash1D(n);
}

// Quintic fade curve
// 6t^5 - 15t^4 + 10t^3
float fade5(float t)
{
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// cubic fade curve
// 3t^2 - 2t^3
float fade3(float t)
{
  return t * t * (3.0f - 2.0f * t);
}

// -1..1 output
float valueNoise2D(float x, float y)
{
  const int x0 = (int)math::floord(x);
  const int y0 = (int)math::floord(y);

  const float tx = x - (float)x0;
  const float ty = y - (float)y0;

  const float sx = fade3(tx);
  const float sy = fade3(ty);

  const float v00 = hash2D(x0, y0);
  const float v10 = hash2D(x0 + 1, y0);
  const float v01 = hash2D(x0, y0 + 1);
  const float v11 = hash2D(x0 + 1, y0 + 1);

  const float ix0 = v00 + (v10 - v00) * sx;
  const float ix1 = v01 + (v11 - v01) * sx;
  return ix0 + (ix1 - ix0) * sy;
}

float fbm2D(float x, float y)
{
  float sum = 0.0f;
  float amp = 1.0f;
  float freq = 1.0f;
  float norm = 0.0f;

  for (int i = 0; i < 3; ++i)
  {
    sum += valueNoise2D(x * freq, y * freq) * amp;
    norm += amp;
    freq *= 2.0f;
    amp *= 0.5f;
  }

  CCASSERT(norm > 0.0001f);

  return sum / norm;
}


}  // namespace WaveSabreCore::M7
