
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
// using math::bilerp() loses minified bytes (20b)
// using FloatPair also loses a bit.
float valueNoise2D(float x, float y)
{
  const int x0 = (int)math::floor(x);
  const float tx = x - (float)x0;
  const float sx = tx;//fade3(tx);

  const int y0 = (int)math::floor(y);
  const float ty = y - (float)y0;
  const float sy = ty;//fade3(ty);

  const float v00 = hash2D(x0, y0);
  const float v10 = hash2D(x0 + 1, y0);
  const float v01 = hash2D(x0, y0 + 1);
  const float v11 = hash2D(x0 + 1, y0 + 1);

  const float ix0 = math::lerp(v00, v10, sx);
  const float ix1 = math::lerp(v01, v11, sx);
  return math::lerp(ix0, ix1, sy);
}

float fbm2D(const FloatPair& p)
{
  float sum = 0.0f;
  float norm = 0.0f;
  float amp = 1.0f;
  float freq = 1.0f;

  for (int i = 0; i < 4; ++i)
  {
    //sum += valueNoise2D(p * freq) * amp;
    sum += valueNoise2D(p.x[0] * freq, p.x[1] * freq) * amp;
    norm += amp;
    freq *= 2.0f;
    amp *= 0.5f;
  }

  CCASSERT(norm > 0.0001f);

  return sum / norm;
}


}  // namespace WaveSabreCore::M7
