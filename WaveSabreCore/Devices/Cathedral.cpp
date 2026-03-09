
#include "Cathedral.h"

namespace WaveSabreCore
{
Cathedral::Cathedral()
    : Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults)
    , mParams{mParamCache, 0}
{
  LoadDefaults();
}

void Cathedral::Run(float** inputs, float** outputs, int numSamples)
{
  float dryMul = mParams.GetLinearVolume(ParamIndices::DryOut, M7::gVolumeCfg12db);
  float wetMul = mParams.GetLinearVolume(ParamIndices::WetOut, M7::gVolumeCfg12db);

  //static constexpr float SVQ = 1;

  for (int s = 0; s < numSamples; s++)
  {
    float leftInput = inputs[0][s];
    float rightInput = inputs[1][s];
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    if (IsGuiVisible())
    {
      mInputAnalysis[0].WriteSample(leftInput);
      mInputAnalysis[1].WriteSample(rightInput);
    }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

    M7::FloatPair dry = {leftInput, rightInput};
    auto wet = mCore.ProcessSample(dry);
    auto outp = M7::FloatPair::Mix(dry, wet, dryMul, wetMul);

    outputs[0][s] = outp.Left();
    outputs[1][s] = outp.Right();
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    if (IsGuiVisible())
    {
      mOutputAnalysis[0].WriteSample(outputs[0][s]);
      mOutputAnalysis[1].WriteSample(outputs[1][s]);
    }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  }
}

void Cathedral::OnParamsChanged()
{
  mCore.preDelayMS = mParams.GetScaledRealValue(
      ParamIndices::PreDelay, 0, 500);  //mParams.Get01Value(ParamIndices::PreDelay) * 500.0f;
  mCore.damp = mParams.Get01Value(ParamIndices::Damp);
  mCore.width = mParams.Get01Value(ParamIndices::Width);
  mCore.lowCutFreq = mParams.GetFrequency(ParamIndices::LowCutFreq, M7::gFilterFreqConfig);
  mCore.highCutFreq = mParams.GetFrequency(ParamIndices::HighCutFreq, M7::gFilterFreqConfig);

  mCore.roomSize = mParams.GetInvDivCurvedValue(ParamIndices::RoomSize, M7::gRoomSizeParamCfg);

  mCore.Update();
}

}  // namespace WaveSabreCore
