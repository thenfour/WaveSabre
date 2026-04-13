#pragma once

#include <Shlwapi.h>
#include <Windows.h>
#include <exception>
#include <fstream>
#include <iostream>

#include <mmreg.h> // must be before MSAcm.h
#include <MSAcm.h>


#pragma comment(lib, "shlwapi.lib")

#include <WaveSabreCore.h>
#include <WaveSabreCore/../../GigaSynth/Maj7.hpp>
#include <WaveSabreCore/../../GigaSynth/GsmSample.h>
#include <WaveSabreVstLib.h>


#include "Maj7Vst.h"

using namespace WaveSabreVstLib;
using namespace WaveSabreCore;

class Maj7Editor : public VstEditor
{
  Maj7Vst* mpMaj7VST;
  M7::Maj7* pMaj7;
  static constexpr int gSamplerWaveformWidth = 700;
  static constexpr int gSamplerWaveformHeight = 75;
  std::vector<std::pair<std::string, int>> mGmDlsOptions;
  bool mShowingGmDlsList = false;
  char mGmDlsFilter[100] = {0};

  struct WaveformPreviewCacheEntry
  {
    M7::OscillatorWaveform waveform = M7::OscillatorWaveform::DefaultForAudio;
    int width = 0;
    int height = 0;
    float waveshapeA01 = 0.0f;
    float waveshapeB01 = 0.0f;
    float phaseOffsetN11 = 0.0f;
    std::vector<float> samples;
    float minY = 0.0f;
    float maxY = 0.0f;
  };

  static constexpr size_t gWaveformPreviewCacheMaxEntries = 64;
  std::vector<WaveformPreviewCacheEntry> mWaveformPreviewCache;
  size_t mWaveformPreviewCacheEvictIndex = 0;

  struct FilterPreviewCacheEntry
  {
    int filterIndex = -1;
    int width = 0;
    int height = 0;
    M7::FilterResponse response = M7::FilterResponse::Lowpass;
    float cutoffHz = 0.0f;
    float reso01 = 0.0f;
    std::vector<float> y01;
  };

  static constexpr size_t gFilterPreviewCacheMaxEntries = 16;
  std::vector<FilterPreviewCacheEntry> mFilterPreviewCache;
  size_t mFilterPreviewCacheEvictIndex = 0;

public:
  struct RenderContext
  {
    M7::Maj7* mpMaj7;
    M7::ModMatrixNode* mpModMatrix = nullptr;
    M7::Maj7::Maj7Voice* mpActiveVoice = nullptr;
    float mModSourceValues[(int)M7::ModSource::Count];
    ImGuiKnobs::ModInfo mModDestValues[(int)M7::ModDestination::Count];

    static M7::Maj7::Maj7Voice* FindRunningVoice(M7::Maj7* pMaj7)
    {
      M7::Maj7::Maj7Voice* playingVoiceToReturn = nullptr;  // pMaj7->mMaj7Voice[0];
      for (auto* v : pMaj7->mMaj7Voice)
      {
        if (!v->IsPlaying())
          continue;
        if (!playingVoiceToReturn || (v->mNoteInfo.mSequence > playingVoiceToReturn->mNoteInfo.mSequence))
        {
          playingVoiceToReturn = v;
        }
      }
      return playingVoiceToReturn;
    }


    void Init(M7::Maj7* pMaj7)
    {
      mpMaj7 = pMaj7;
      mpActiveVoice = FindRunningVoice(pMaj7);
      // dont hate me
      memset(mModSourceValues, 0, sizeof(mModSourceValues));
      memset(mModDestValues, 0, sizeof(mModDestValues));
      if (mpActiveVoice)
      {
        mpModMatrix = &mpActiveVoice->mModMatrix;
        for (int i = 0; i < (int)M7::ModSource::Count; ++i)
        {
          mModSourceValues[i] = mpActiveVoice->mModMatrix.GetSourceValue(i);
        }
        for (int i = 0; i < (int)M7::ModDestination::Count; ++i)
        {
          mModDestValues[i].mValue = mpActiveVoice->mModMatrix.GetDestinationValue(i);
        }
      }
      else
      {
        mpModMatrix = &pMaj7->mMaj7Voice[0]->mModMatrix;
      }

      for (auto* m : pMaj7->mpModulations)
      {
        if (!m->mEnabled)
          continue;
        if (m->mSource == M7::ModSource::None)
          continue;
        for (size_t i = 0; i < std::size(m->mDestinations); ++i)
        {
          mModDestValues[(int)m->mDestinations[i]].mEnabled = true;
          // extents
          mModDestValues[(int)m->mDestinations[i]].mExtentMax += std::abs(m->mScales[i]);
          mModDestValues[(int)m->mDestinations[i]].mExtentMin = mModDestValues[(int)m->mDestinations[i]].mExtentMax;
        }
      }
    }
  };

  RenderContext mRenderContext;
  ImGuiTabSelectionHelper mSourceTabSelHelper;
  ImGuiTabSelectionHelper mModEnvOrLFOTabSelHelper;
  ImGuiTabSelectionHelper mFilterTabSelHelper;
  ImGuiTabSelectionHelper mModulationTabSelHelper;

  Maj7Editor(AudioEffect* audioEffect)
      : VstEditor(audioEffect, 1250, 990)
      , mpMaj7VST((Maj7Vst*)audioEffect)
      , pMaj7(((Maj7Vst*)audioEffect)->GetMaj7())
  {
    mGmDlsOptions = LoadGmDlsOptions();
    mSourceTabSelHelper.mpSelectedTabIndex = &mpMaj7VST->mSelectedSource;
    mModEnvOrLFOTabSelHelper.mpSelectedTabIndex = &mpMaj7VST->mSelectedModEnvOrLFO;
    mFilterTabSelHelper.mpSelectedTabIndex = &mpMaj7VST->mSelectedFilter;
    mModulationTabSelHelper.mpSelectedTabIndex = &mpMaj7VST->mSelectedModulation;
  }

  enum class StatusStyle
  {
    NoStyle,
    Green,
    Warning,
    Error,
  };

  struct SourceStatusText
  {
    std::string mStatus = "Ready.";
    StatusStyle mStatusStyle = StatusStyle::NoStyle;
  };

  SourceStatusText mSourceStatusText[M7::gSourceCount];

  bool SetStatus(size_t isrc, StatusStyle style, const std::string& str)
  {
    mSourceStatusText[isrc].mStatusStyle = style;
    mSourceStatusText[isrc].mStatus = str;
    return false;
  }


  void Sort(float& a, float& b)
  {
    float low = std::min(a, b);
    b = std::max(a, b);
    a = low;
  }

  std::tuple<bool, float, float> Intersect(float a1, float a2, float b1, float b2)
  {
    Sort(a1, a2);
    Sort(b1, b2);
    if (a1 <= b2 && a2 >= b1)
    {
      // there is an intersection.
      return std::make_tuple(true, std::max(b1, a1), std::min(b2, a2));
    }
    return std::make_tuple(false, 0.0f, 0.0f);
  }

  void TooltipF(const char* format, ...)
  {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

      char buffer[1024];

      va_list args;
      va_start(args, format);
      vsnprintf(buffer, sizeof(buffer), format, args);
      va_end(args);

      ImGui::TextUnformatted(buffer);
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
  }


  void OnLoadPatch()
  {
    OPENFILENAMEA ofn = {0};
    char szFile[MAX_PATH] = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = this->mCurrentWindow;
    ofn.lpstrFilter = "Maj7 patches (*.majson;*.json;*.js;*.txt)\0*.majson;*.json;*.js;*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn))
    {
      // load file contents
      std::string s = LoadContentsOfTextFile(szFile);
      if (!s.empty())
      {
        // todo: use an internal setchunk and report load errors
        mpMaj7VST->setChunk((void*)s.c_str(), VstInt32(s.size() + 1), false);  // const cast oooooooh :/

        // reset voice states etc.
        auto p = mpMaj7VST->GetMaj7();

        for (auto& s : p->mSources)
        {
          s->InitDevice();
        }

        ::MessageBoxA(mCurrentWindow, "Loaded successfully.", "WaveSabre - Maj7", MB_OK | MB_ICONINFORMATION);
      }
    }
  }

  bool SaveToFile(const std::string& path, const std::string& textContents)
  {
    if (!textContents.size())
      return false;
    HANDLE fileHandle = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
      return false;
    }

    DWORD bytesWritten;
    DWORD bytesToWrite = (DWORD)(textContents.size() * sizeof(textContents[0]));
    BOOL success = WriteFile(fileHandle, textContents.c_str(), bytesToWrite, &bytesWritten, NULL);

    CloseHandle(fileHandle);
    return (success != 0) && (bytesWritten == static_cast<DWORD>(bytesToWrite));
  }

  // Function to show a "Save As" dialog box and get the selected file path.
  std::string GetSaveFilePath(HWND hwnd)
  {
    static constexpr size_t gMaxPathLen = 1000;
    auto buf = std::make_unique<char[]>(gMaxPathLen);

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Maj7 patches (*.majson;*.json;*.js;*.txt)\0*.majson;*.json;*.js;*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrDefExt = "majson";
    ofn.lpstrFile = buf.get();
    ofn.nMaxFile = gMaxPathLen;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    //ofn.lpstrFileTitle = defaultFileName;
    //ofn.nMaxFileTitle = lstrlen(defaultFileName);

    if (GetSaveFileName(&ofn) == TRUE)
    {
      std::string ret = buf.get();
      return ret;
    }

    return "";
  }


  void OnSavePatchPatchAs()
  {
    auto path = GetSaveFilePath(this->mCurrentWindow);
    if (path.empty())
      return;

    void* data;
    int size;
    std::string contents;
    size = mpMaj7VST->getChunk2(&data, false, false);
    if (data && size)
    {
      contents = (const char*)data;
    }
    delete[] data;

    if (!contents.empty())
    {
      SaveToFile(path, contents);
      ::MessageBoxA(mCurrentWindow, "Saved successfully.", "WaveSabre - Maj7", MB_OK | MB_ICONINFORMATION);
    }
  }


  virtual void PopulateMenuBar() override
  {
    if (ImGui::BeginMenu("[Maj7]"))
    {
      bool b = false;

      if (ImGui::MenuItem("Panic", nullptr, false))
      {
        pMaj7->AllNotesOff();
      }

      ImGui::Separator();

      ImGui::MenuItem("Show performance window", nullptr, &GetEffectX()->mShowingPerformanceWindow);

      if (ImGui::BeginMenu("Performance"))
      {
        QUALITY_SETTING_CAPTIONS(captions);
        size_t currentSelectionID = (size_t)M7::GetQualitySetting();
        int newSelection = -1;
        for (size_t i = 0; i < (size_t)M7::QualitySetting::Count; ++i)
        {
          bool selected = (currentSelectionID == i);
          if (ImGui::MenuItem(captions[i], nullptr, &selected))
          {
            newSelection = (int)i;
          }
        }
        if (newSelection >= 0)
        {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          M7::SetQualitySetting((M7::QualitySetting)newSelection);
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
        }
        ImGui::EndMenu();
      }

      ImGui::Separator();
      if (ImGui::MenuItem("Init patch"))
      {
        pMaj7->LoadDefaults();
        mpMaj7VST->SetDefaultSettings();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Load patch..."))
      {
        OnLoadPatch();
      }
      if (ImGui::MenuItem("Save patch as..."))
      {
        OnSavePatchPatchAs();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Copy patch to clipboard"))
      {
        CopyPatchToClipboard(false);
      }

      if (ImGui::MenuItem("Paste patch from clipboard"))
      {
        std::string s = GetClipboardText();
        if (!s.empty())
        {
          mpMaj7VST->setChunk((void*)s.c_str(), VstInt32(s.size() + 1), false);  // const cast oooooooh :/
        }
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Documentation", nullptr, false))
      {
        ShellExecute(NULL,
                     "open",
                     "https://github.com/thenfour/WaveSabre/blob/maj7_documentation/Docs/Maj7.md",
                     NULL,
                     NULL,
                     SW_SHOWNORMAL);
      }

      if (ImGui::MenuItem("WaveSabre Github", nullptr, false))
      {
        ShellExecute(NULL, "open", "https://github.com/logicomacorp/WaveSabre", NULL, NULL, SW_SHOWNORMAL);
      }

      if (ImGui::MenuItem("Maj7 was written by by tenfour", nullptr, false))
      {
        ShellExecute(NULL, "open", "https://tenfourmusic.net/", NULL, NULL, SW_SHOWNORMAL);
      }

      ImGui::EndMenu();
    }  // maj7 menu

    if (ImGui::BeginMenu("[Debug]"))
    {
      if (ImGui::MenuItem("Fix amp envelope mods"))
      {
        auto p = mpMaj7VST->GetMaj7();

        for (auto& s : p->mSources)
        {
          s->InitDevice();
        }
      }

      if (ImGui::MenuItem("Show polyphonic inspector", nullptr, &mShowingInspector))
      {
        if (!mShowingInspector)
          mShowingModulationInspector = false;
      }
      if (ImGui::MenuItem("Show modulation inspector", nullptr, &mShowingModulationInspector))
      {
        if (mShowingModulationInspector)
          mShowingInspector = true;
      }
      ImGui::MenuItem("Show color expl", nullptr, &mShowingColorExp);

      ImGui::Separator();

      if (ImGui::MenuItem("Init patch (from VST)"))
      {
        if (IDYES == ::MessageBoxA(mCurrentWindow,
                                   "Sure? This will clobber your patch, and generate defaults, to be used with the "
                                   "next menu item to generate Maj7.cpp. ",
                                   "WaveSabre - Maj7",
                                   MB_YESNO | MB_ICONQUESTION))
        {
           mpMaj7VST->GenerateDefaults();
        }
      }

      if (ImGui::MenuItem("Export as Maj7.cpp defaults to clipboard"))
      {
        if (IDYES == ::MessageBoxA(mCurrentWindow,
                                   "A new maj7.cpp will be copied to the clipboard, based on 1st item params.",
                                   "WaveSabre - Maj7",
                                   MB_YESNO | MB_ICONQUESTION))
        {
          CopyParamCache();
        }
      }

      if (ImGui::MenuItem("Copy DIFF patch to clipboard"))
      {
        CopyPatchToClipboard(true);
      }


      if (ImGui::MenuItem("Test chunk roundtrip"))
      {
        if (IDYES == ::MessageBoxA(mCurrentWindow,
                                   "Sure? This could ruin your patch. But hopefully it doesn't?",
                                   "WaveSabre - Maj7",
                                   MB_YESNO | MB_ICONQUESTION))
        {
          // make a copy of param cache.
          float orig[(size_t)M7::GigaSynthParamIndices::NumParams];
          for (size_t i = 0; i < (size_t)M7::GigaSynthParamIndices::NumParams; ++i)
          {
            orig[i] = pMaj7->GetParam((int)i);
          }

          // get the wavesabre chunk

          void* data;
          mpMaj7VST->OptimizeParams();
          int n = mpMaj7VST->GetMinifiedChunk(&data, true);
          //auto defaultParamCache = GetDefaultParamCache();
          //int n = pMaj7->GetChunk(&data);
          //int n = mpMaj7VST->GetMinifiedChunk(&data);
          //int n = GetMinifiedChunk(pMaj7, &data, defaultParamCache);
          //pMaj7->SetChunk(data, n);

          // apply it back (round trip)
          M7::Deserializer ds{(const uint8_t*)data};
          pMaj7->SetBinary16DiffChunk(ds);
          delete[] data;


          float after[(size_t)M7::GigaSynthParamIndices::NumParams];
          for (size_t i = 0; i < (size_t)M7::GigaSynthParamIndices::NumParams; ++i)
          {
            after[i] = pMaj7->GetParam((int)i);
          }

          //float delta = 0;
          std::vector<std::string> paramReports;

          using vstn = const char[kVstMaxParamStrLen];
          static constexpr vstn paramNames[(int)M7::GigaSynthParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

          for (size_t i = 0; i < (size_t)M7::GigaSynthParamIndices::NumParams; ++i)
          {
            if (!M7::math::FloatEquals(orig[i], after[i]))
            {
              //delta = abs(orig[i] - after[i]);
              //bdiff++;
              char msg[200];
              sprintf_s(msg, "%s before=%.2f after=%.2f", paramNames[i], orig[i], after[i]);
              paramReports.push_back(msg);
            }
          }

          char msg[200];
          sprintf_s(msg, "Done. %d bytes long. %d params have been messed up.\r\n", n, (int)paramReports.size());
          std::string smsg{msg};
          smsg += "\r\n";
          for (auto& p : paramReports)
          {
            smsg += p;
          }
          ::MessageBoxA(mCurrentWindow, smsg.c_str(), "WaveSabre - Maj7", MB_OK);
        }
      }


      if (ImGui::MenuItem("Optimize"))
      {
        // the idea is to reset any unused parameters to default values, so they end up being 0 in the minified chunk.
        // that compresses better. this is a bit tricky though; i guess i should only do this for like, samplers, oscillators, modulations 1-8, and all envelopes.
        if (IDYES == ::MessageBoxA(mCurrentWindow,
                                   "Unused objects will be clobbered; are you sure? Do this as a post-processing step "
                                   "before rendering the minified song.",
                                   "WaveSabre - Maj7",
                                   MB_YESNO | MB_ICONQUESTION))
        {
          if (IDYES == ::MessageBoxA(mCurrentWindow,
                                     "Backup current patch to clipboard?",
                                     "WaveSabre - Maj7",
                                     MB_YESNO | MB_ICONQUESTION))
          {
            CopyPatchToClipboard(false);
            ::MessageBoxA(mCurrentWindow,
                          "Copied to clipboard... click OK to continue to optimization",
                          "WaveSabre - Maj7",
                          MB_OK);
          }

          //auto defaultParamCache = GetDefaultParamCache();

          auto r1 = mpMaj7VST->AnalyzeChunkMinification();  // AnalyzeChunkMinification(pMaj7);
          mpMaj7VST->OptimizeParams();
          auto r2 = mpMaj7VST->AnalyzeChunkMinification();  // AnalyzeChunkMinification(pMaj7);
          char msg[200];
          sprintf_s(msg,
                    "Done!\r\nBefore: %d bytes; %d nondefaults\r\nAfter: %d bytes; %d nondefaults\r\nShrunk to %d %%",
                    r1.compressedSize,
                    r1.nonZeroParams,
                    r2.compressedSize,
                    r2.nonZeroParams,
                    int(((float)r2.compressedSize / r1.compressedSize) * 100));
          ::MessageBoxA(mCurrentWindow, msg, "WaveSabre - Maj7", MB_OK);
        }
      }
      if (ImGui::MenuItem("Analyze minified chunk"))
      {
        //auto defaultParamCache = GetDefaultParamCache();
        auto r = mpMaj7VST->AnalyzeChunkMinification();  // AnalyzeChunkMinification(pMaj7);
        std::string s = "uncompressed = ";
        s += std::to_string(r.uncompressedSize);
        s += " bytes.\r\nLZMA compressed this to ";
        s += std::to_string(r.compressedSize);
        s += " bytes.\r\nNon-default params set: ";
        s += std::to_string(r.nonZeroParams);
        s += "\r\nDefault params : ";
        s += std::to_string(r.defaultParams);
        ::MessageBoxA(mCurrentWindow, s.c_str(), "WaveSabre - Maj7", MB_OK);
      }

      ImGui::EndMenu();
    }  // debug menu
  }

  void CopyPatchToClipboard(bool diff)
  {
    void* data;
    int size;
    size = mpMaj7VST->getChunk2(&data, false, diff);
    if (data && size)
    {
      CopyTextToClipboard((const char*)data);
    }
    delete[] data;
    ::MessageBoxA(mCurrentWindow, "Copied patch to clipboard", "WaveSabre - Maj7", MB_OK | MB_ICONINFORMATION);
  }

  void CopyParamCache()
  {
    using vstn = const char[kVstMaxParamStrLen];
    static constexpr vstn paramNames[(int)M7::GigaSynthParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;
    std::stringstream ss;
    ss << "#include \"../GigaSynth/GigaParams.hpp\"" << std::endl;
    ss << "namespace WaveSabreCore {" << std::endl;
    ss << "  namespace M7 {" << std::endl;

    auto GenerateArray = [&](const std::string& arrayName, size_t count, const std::string& countExpr, int baseParamID)
    {
      ss << "    static_assert((int)" << countExpr << " == " << count
         << ", \"param count probably changed and this needs to be regenerated.\");" << std::endl;
      ss << "    extern const int16_t " << arrayName << "[" << count << "] = {" << std::endl;
      for (size_t i = 0; i < count; ++i)
      {
        size_t paramID = baseParamID + i;
        float valf = GetEffectX()->getParameter((VstInt32)paramID);
        ss << std::setprecision(20) << "      " << M7::math::FloatN11ToDefault16(valf) << ", // " << paramNames[paramID]
           << " = " << valf << std::endl;
      }
      ss << "    };" << std::endl;
    };

    GenerateArray("gDefaultMasterParams", (int)M7::MainParamIndices::Count, "M7::MainParamIndices::Count", 0);
    GenerateArray("gDefaultSamplerParams",
                  (int)M7::SamplerParamIndexOffsets::Count,
                  "M7::SamplerParamIndexOffsets::Count",
                  (int)pMaj7->mpSamplerDevices[0]->mParams.mBaseParamID);
    GenerateArray("gDefaultModSpecParams",
                  (int)M7::ModParamIndexOffsets::Count,
                  "M7::ModParamIndexOffsets::Count",
                  (int)pMaj7->mpModulations[0]->mParams.mBaseParamID);
    GenerateArray("gDefaultLFOParams",
                  (int)M7::LFOParamIndexOffsets::Count,
                  "M7::LFOParamIndexOffsets::Count",
                  (int)pMaj7->mpLFOs[0]->mDevice.mParams.mBaseParamID);
    GenerateArray("gDefaultEnvelopeParams",
                  (int)M7::EnvParamIndexOffsets::Count,
                  "M7::EnvParamIndexOffsets::Count",
                  (int)pMaj7->mMaj7Voice[0]->mpEnvelopes[M7::gOsc1AmpEnvIndex]->mParams.mBaseParamID);
    GenerateArray("gDefaultOscillatorParams",
                  (int)M7::OscParamIndexOffsets::Count,
                  "M7::OscParamIndexOffsets::Count",
                  (int)pMaj7->mpOscillatorDevices[0]->mParams.mBaseParamID);
    GenerateArray("gDefaultFilterParams",
                  (int)M7::FilterParamIndexOffsets::Count,
                  "M7::FilterParamIndexOffsets::Count",
                  (int)M7::GigaSynthParamIndices::Filter1Enabled);

    ss << "  } // namespace M7" << std::endl;
    ss << "} // namespace WaveSabreCore" << std::endl;
    ImGui::SetClipboardText(ss.str().c_str());

    ::MessageBoxA(mCurrentWindow, "Copied new contents of WaveSabreCore/Maj7.cpp", "WaveSabre Maj7", MB_OK);
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual std::string GetMenuBarStatusText() override
  {
    std::string ret = " Voices:";
    ret += std::to_string(pMaj7->GetCurrentPolyphony());

    //auto* v = FindRunningVoice();
    if (mRenderContext.mpActiveVoice)
    {
      ret += " ";
      ret += midiNoteToString(mRenderContext.mpActiveVoice->mNoteInfo.MidiNoteValue);
    }

    return ret;
  };
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  void Maj7ImGuiParamMacro(int imacro)
  {
    int paramID = (int)M7::GigaSynthParamIndices::Macro1 + imacro;
    float tempVal = GetEffectX()->getParameter((VstInt32)paramID);
    //M7::Float01Param p{ tempVal };
    //M7::ParamAccessor pa{ &tempVal, 0 };
    float defaultParamVal = 0;  // p.Get01Value();
    //p.SetParamValue();
    float centerVal01 = 0;
    Float01Converter conv{};

    std::string label;
    std::string id = "##macro";
    id += std::to_string(imacro);

    if (mpMaj7VST->mMacroNames[imacro].empty())
    {
      std::string defaultLabel = "Macro " + std::to_string(imacro + 1);
      label = defaultLabel;
    }
    else
    {
      label = mpMaj7VST->mMacroNames[imacro];
    }

    if (ImGuiKnobs::KnobWithEditableLabel(label,
                                          id.c_str(),
                                          &tempVal,
                                          0,
                                          1,
                                          defaultParamVal,
                                          centerVal01,
                                          {},
                                          gNormalKnobSpeed,
                                          gSlowKnobSpeed,
                                          nullptr,
                                          ImGuiKnobVariant_WiperOnly,
                                          0,
                                          ImGuiKnobFlags_CustomInput | ImGuiKnobFlags_EditableTitle,
                                          10,
                                          &conv,
                                          this))
    {
      GetEffectX()->setParameterAutomated(paramID, (tempVal));
    }
    if (label != mpMaj7VST->mMacroNames[imacro])
    {
      mpMaj7VST->mMacroNames[imacro] = label;
    }
  }

  ColorMod mModulationsColors{0.15f, 0.6f, 0.65f, 0.9f, 0.0f};
  ColorMod mModulationDisabledColors{0.15f, 0.0f, 0.65f, 0.6f, 0.0f};
  ColorMod mLockedModulationsColors{0.50f, 0.6f, 0.75f, 0.9f, 0.0f};

  ColorMod mModEnvelopeColors{0.75f, 0.83f, 0.86f, 0.95f, 0.0f};

  ColorMod mLFOColors{0.4f, 0.6f, 0.65f, 0.9f, 0.0f};

  ColorMod mAuxLeftColors{0.1f, 1, 1, .9f, 0.5f};
  ColorMod mAuxRightColors{0.4f, 1, 1, .9f, 0.5f};
  ColorMod mAuxLeftDisabledColors{0.1f, 0.25f, .4f, .5f, 0.1f};
  ColorMod mAuxRightDisabledColors{0.4f, 0.25f, .4f, .5f, 0.1f};

  ColorMod mOscColors{0, 1, 1, 0.9f, 0.0f};
  ColorMod mOscDisabledColors{0, .15f, .6f, 0.5f, 0.2f};

  ColorMod mSamplerColors{0.55f, 0.8f, .9f, 1.0f, 0.5f};
  ColorMod mSamplerDisabledColors{0.55f, 0.15f, .6f, 0.5f, 0.2f};

  ColorMod mFMColors{0.92f, 0.6f, 0.75f, 0.9f, 0.0f};
  ColorMod mFMFeedbackColors{0.92f - 0.5f, 0.6f, 0.75f, 0.9f, 0.0f};

  ColorMod mFMDisabledColors{0.92f, 0.0f, 0.3f, 0.6f, 0.0f};
  ColorMod mFMDisabledFeedbackColors{0.92f - 0.5f, 0.0f, 0.3f, 0.6f, 0.0f};

  ColorMod mNopColors;

  bool mShowingInspector = false;
  bool mShowingModulationInspector = false;
  bool mShowingColorExp = false;
  //bool mShowingLockedModulations = false;


  virtual void renderImgui() override
  {
    mModulationsColors.EnsureInitialized();
    mModulationDisabledColors.EnsureInitialized();
    mFMColors.EnsureInitialized();
    mFMFeedbackColors.EnsureInitialized();
    mFMDisabledColors.EnsureInitialized();
    mFMDisabledFeedbackColors.EnsureInitialized();
    mLockedModulationsColors.EnsureInitialized();
    mOscColors.EnsureInitialized();
    mOscDisabledColors.EnsureInitialized();
    mAuxLeftColors.EnsureInitialized();
    mAuxRightColors.EnsureInitialized();
    mAuxLeftDisabledColors.EnsureInitialized();
    mAuxRightDisabledColors.EnsureInitialized();
    mSamplerDisabledColors.EnsureInitialized();
    mSamplerColors.EnsureInitialized();
    mLFOColors.EnsureInitialized();
    mModEnvelopeColors.EnsureInitialized();

    mRenderContext.Init(this->pMaj7);

    {
      auto& style = ImGui::GetStyle();
      style.FramePadding.x = 7;
      style.FramePadding.y = 3;
      style.ItemSpacing.x = 6;
      style.TabRounding = 3;
    }

    // color explorer
    ColorMod::Token colorExplorerToken;
    if (mShowingColorExp)
    {
      static float colorHueVarAmt = 0;
      static float colorSaturationVarAmt = 1;
      static float colorValueVarAmt = 1;
      static float colorTextVal = 0.9f;
      static float colorTextDisabledVal = 0.5f;
      bool b1 = ImGuiKnobs::Knob(
          "hue", &colorHueVarAmt, -1.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
      ImGui::SameLine();
      bool b2 = ImGuiKnobs::Knob(
          "sat", &colorSaturationVarAmt, 0.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
      ImGui::SameLine();
      bool b3 = ImGuiKnobs::Knob(
          "val", &colorValueVarAmt, 0.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
      ImGui::SameLine();
      bool b4 = ImGuiKnobs::Knob(
          "txt", &colorTextVal, 0.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
      ImGui::SameLine();
      bool b5 = ImGuiKnobs::Knob(
          "txtD", &colorTextDisabledVal, 0.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
      ColorMod xyz{colorHueVarAmt, colorSaturationVarAmt, colorValueVarAmt, colorTextVal, colorTextDisabledVal};
      xyz.EnsureInitialized();
      colorExplorerToken = std::move(xyz.Push());
    }

    //auto runningVoice = FindRunningVoice();

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    MAJ7_OUTPUT_STREAM_CAPTIONS(outputStreamCaptions);
    auto elementCount = std::size(outputStreamCaptions);
    for (int iOutput = 0; iOutput < 2; ++iOutput)
    {
      ImGui::PushID(iOutput);
      if (iOutput > 0)
        ImGui::SameLine();
      ImGui::Text("Output stream %d", iOutput);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(200);
      if (ImGui::BeginCombo("##outputStream", outputStreamCaptions[(int)pMaj7->mOutputStreams[iOutput]]))
      {
        auto selectedIndex = (int)pMaj7->mOutputStreams[iOutput];
        for (int n = 0; n < (int)elementCount; n++)
        {
          const bool is_selected = (selectedIndex == n);
          if (ImGui::Selectable(outputStreamCaptions[n], is_selected))
          {
            pMaj7->mOutputStreams[iOutput] = (decltype(pMaj7->mOutputStreams[iOutput]))n;
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopID();
    }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    Maj7ImGuiParamVolume((VstInt32)M7::GigaSynthParamIndices::MasterVolume, "Volume##hc", M7::gMasterVolumeCfg, -6.0f, {});

    ImGui::SameLine();
    Maj7ImGuiParamFloatN11((VstInt32)M7::GigaSynthParamIndices::Pan, "Pan##mst", 0.0f, 0, GetModInfo(M7::ModDestination::Pan));

    ImGui::SameLine();
    Maj7ImGuiParamInt((VstInt32)M7::GigaSynthParamIndices::Unisono, "Unison##mst", M7::gUnisonoVoiceCfg, 1, 0);

    //ImGui::SameLine();
    //WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorDetune, "OscDetune##mst");
    ImGui::SameLine();
    WSImGuiParamKnob((VstInt32)M7::GigaSynthParamIndices::UnisonoDetune, "UniDetune##mst");
    //ImGui::SameLine();
    //WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorSpread, "OscPan##mst");
    ImGui::SameLine();
    WSImGuiParamKnob((VstInt32)M7::GigaSynthParamIndices::UnisonoStereoSpread, "UniPan##mst");

    ImGui::SameLine(0, 60);
    Maj7ImGuiParamInt((VstInt32)M7::GigaSynthParamIndices::PitchBendRange, "PB Range##mst", M7::gPitchBendCfg, 2, 0);
    ImGui::SameLine();

    // auto pbVal = GetEffectX()->getParameter((VstInt32)M7::GigaSynthParamIndices::PitchBendRange);
    // ImGui::Text("%.10f", pbVal);
    // ImGui::SameLine();

    Maj7ImGuiPowCurvedParam((VstInt32)M7::GigaSynthParamIndices::PortamentoTime,
                            "Port##mst",
                            M7::gEnvTimeCfg,
                            0.4f,
                            GetModInfo(M7::ModDestination::PortamentoTime));

    //ImGui::SameLine();
    //Maj7ImGuiParamCurve((VstInt32)M7::ParamIndices::PortamentoCurve, "##portcurvemst", 0.0f, M7CurveRenderStyle::Rising, {});
    ImGui::SameLine();
    Maj7ImGuiParamInt((VstInt32)M7::GigaSynthParamIndices::MaxVoices, "MaxVox", M7::gMaxVoicesCfg, 24, 1);

    static constexpr char const* const voiceModeCaptions[] = {"Poly", "Mono"};
    ImGui::SameLine(0, 60);
    Maj7ImGuiParamEnumList<WaveSabreCore::VoiceMode>((VstInt32)M7::GigaSynthParamIndices::VoicingMode,
                                                     "VoiceMode##mst",
                                                     (int)WaveSabreCore::VoiceMode::Count,
                                                     WaveSabreCore::VoiceMode::Polyphonic,
                                                     voiceModeCaptions);

    //AUX_ROUTE_CAPTIONS(auxRouteCaptions);
    //ImGui::SameLine(); Maj7ImGuiParamEnumCombo((VstInt32)M7::ParamIndices::AuxRouting, "AuxRoute", (int)M7::AuxRoute::Count, M7::AuxRoute::TwoTwo, auxRouteCaptions, 100);
    //ImGui::SameLine(); Maj7ImGuiParamFloatN11((VstInt32)M7::ParamIndices::AuxWidth, "AuxWidth", 1, 0, {});

    {
      static const std::vector<VUMeterTick> tickSet = {
          {-6.0f, "6db"},
          {-18.0f, "18db"},
          {-30.0f, "30db"},
      };

      VUMeterConfig mainCfg = {
          {24, 70},
          VUMeterLevelMode::Audio,
          VUMeterUnits::Linear,
          -40,
          6,
          tickSet,
      };

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

      ImGui::SameLine(0, 160);
  {
    VUMeterTooltipStripScope tooltipStrip{"gigasynth_output_vu_strip"};
    VUMeter("outputVU",
        pMaj7->mOutputAnalysis[0],
        pMaj7->mOutputAnalysis[1],
        mainCfg,
        "Output Left",
        "Output Right",
        &tooltipStrip);
  }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    }

    // osc1
    if (BeginTabBar2("osc", ImGuiTabBarFlags_None))
    {
      static_assert(M7::gOscillatorCount == 4, "osc count");
      int isrc = 0;
      Oscillator("Oscillator 1",
                 (int)M7::GigaSynthParamIndices::Osc1Enabled,
                 isrc++,
                 (int)M7::ModDestination::Osc1AmpEnvDelayTime);
      Oscillator("Oscillator 2",
                 (int)M7::GigaSynthParamIndices::Osc2Enabled,
                 isrc++,
                 (int)M7::ModDestination::Osc2AmpEnvDelayTime);
      Oscillator("Oscillator 3",
                 (int)M7::GigaSynthParamIndices::Osc3Enabled,
                 isrc++,
                 (int)M7::ModDestination::Osc3AmpEnvDelayTime);
      Oscillator("Oscillator 4",
                 (int)M7::GigaSynthParamIndices::Osc4Enabled,
                 isrc++,
                 (int)M7::ModDestination::Osc4AmpEnvDelayTime);

      static_assert(M7::gSamplerCount == 4, "sampler count");
      Sampler("Sampler 1", *pMaj7->mpSamplerDevices[0], isrc++, (int)M7::ModDestination::Sampler1AmpEnvDelayTime);
      Sampler("Sampler 2", *pMaj7->mpSamplerDevices[1], isrc++, (int)M7::ModDestination::Sampler2AmpEnvDelayTime);
      Sampler("Sampler 3", *pMaj7->mpSamplerDevices[2], isrc++, (int)M7::ModDestination::Sampler3AmpEnvDelayTime);
      Sampler("Sampler 4", *pMaj7->mpSamplerDevices[3], isrc++, (int)M7::ModDestination::Sampler4AmpEnvDelayTime);
      EndTabBarWithColoredSeparator();
    }

    ImGui::BeginTable("##fmaux", 3);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    static_assert(M7::gOscillatorCount == 4, "osc count");
    bool oscEnabled[4] = {
        pMaj7->mParams.GetBoolValue(M7::GigaSynthParamIndices::Osc1Enabled),
        pMaj7->mParams.GetBoolValue(M7::GigaSynthParamIndices::Osc2Enabled),
        pMaj7->mParams.GetBoolValue(M7::GigaSynthParamIndices::Osc3Enabled),
        pMaj7->mParams.GetBoolValue(M7::GigaSynthParamIndices::Osc4Enabled),
    };

    //ImGui::BeginGroup();
    if (BeginTabBar2("FM", ImGuiTabBarFlags_None, 330))
    {
      auto colorModToken = mFMColors.Push();
      static_assert(M7::gOscillatorCount == 4, "osc count");
      if (WSBeginTabItem("\"Frequency Modulation\""))
      {
        ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {0, 0});
        {
          auto colorModToken = (oscEnabled[0] ? &mFMFeedbackColors : &mFMDisabledFeedbackColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::Osc1FMFeedback, "FB1");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[0] && oscEnabled[1]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt2to1, "2-1");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[0] && oscEnabled[2]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt3to1, "3-1");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[0] && oscEnabled[3]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt4to1, "4-1");
        }

        {
          auto colorModToken = ((oscEnabled[0] && oscEnabled[1]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt1to2, "1-2");
        }
        {
          auto colorModToken = (oscEnabled[1] ? &mFMFeedbackColors : &mFMDisabledFeedbackColors)->Push();
          ImGui::SameLine();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::Osc2FMFeedback, "FB2");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[1] && oscEnabled[2]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt3to2, "3-2");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[1] && oscEnabled[3]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt4to2, "4-2");
        }

        {
          auto colorModToken = ((oscEnabled[0] && oscEnabled[2]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt1to3, "1-3");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[1] && oscEnabled[2]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt2to3, "2-3");
        }
        {
          auto colorModToken = (oscEnabled[2] ? &mFMFeedbackColors : &mFMDisabledFeedbackColors)->Push();
          ImGui::SameLine();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::Osc3FMFeedback, "FB3");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[3] && oscEnabled[2]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt4to3, "4-3");
        }

        {
          auto colorModToken = ((oscEnabled[0] && oscEnabled[3]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt1to4, "1-4");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[1] && oscEnabled[3]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt2to4, "2-4");
        }
        ImGui::SameLine();
        {
          auto colorModToken = ((oscEnabled[2] && oscEnabled[3]) ? &mFMColors : &mFMDisabledColors)->Push();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::FMAmt3to4, "3-4");
        }
        {
          auto colorModToken = (oscEnabled[3] ? &mFMFeedbackColors : &mFMDisabledFeedbackColors)->Push();
          ImGui::SameLine();
          Maj7ImGuiParamFMKnob((VstInt32)M7::GigaSynthParamIndices::Osc4FMFeedback, "FB4");
        }

        ImGui::PopStyleVar();
        ImGui::EndGroup();

        ImGui::SameLine(0, 20);
        Maj7ImGuiParamFloat01((VstInt32)M7::GigaSynthParamIndices::FMBrightness,
                              "Brightness##mst",
                              0.5f,
                              0.5f,
                              0,
                              GetModInfo(M7::ModDestination::FMBrightness));

        ImGui::EndTabItem();
      }
      EndTabBarWithColoredSeparator();
    }
    //ImGui::EndGroup();

    ImGui::TableNextColumn();

    // aux
    if (BeginTabBar2("aux", ImGuiTabBarFlags_None, 340))
    {
      AuxEffectTab("Filter1");
      EndTabBarWithColoredSeparator();
    }

    ImGui::EndTable();


    // modulation shapes
    if (BeginTabBar2("envelopetabs", ImGuiTabBarFlags_None))
    {
      auto modColorModToken = mModEnvelopeColors.Push();
      if (WSBeginTabItemWithSel("Mod env 1", 0, mModEnvOrLFOTabSelHelper))
      {
        Envelope("Modulation Envelope 1", (int)M7::GigaSynthParamIndices::Env1DelayTime, (int)M7::ModDestination::Env1DelayTime);
        ImGui::EndTabItem();
      }
      if (WSBeginTabItemWithSel("Mod env 2", 1, mModEnvOrLFOTabSelHelper))
      {
        Envelope("Modulation Envelope 2", (int)M7::GigaSynthParamIndices::Env2DelayTime, (int)M7::ModDestination::Env2DelayTime);
        ImGui::EndTabItem();
      }
      if (WSBeginTabItemWithSel("Mod env 3", 2, mModEnvOrLFOTabSelHelper))
      {
        Envelope("Modulation Envelope 3", (int)M7::GigaSynthParamIndices::Env3DelayTime, (int)M7::ModDestination::Env3DelayTime);
        ImGui::EndTabItem();
      }
      if (WSBeginTabItemWithSel("Mod env 4", 3, mModEnvOrLFOTabSelHelper))
      {
        Envelope("Modulation Envelope 4", (int)M7::GigaSynthParamIndices::Env4DelayTime, (int)M7::ModDestination::Env4DelayTime);
        ImGui::EndTabItem();
      }
      auto lfoColorModToken = mLFOColors.Push();
      if (WSBeginTabItemWithSel("LFO 1", 4, mModEnvOrLFOTabSelHelper))
      {
        LFO("LFO 1", (int)M7::GigaSynthParamIndices::LFO1Waveform, 0);
        ImGui::EndTabItem();
      }
      if (WSBeginTabItemWithSel("LFO 2", 5, mModEnvOrLFOTabSelHelper))
      {
        LFO("LFO 2", (int)M7::GigaSynthParamIndices::LFO2Waveform, 1);
        ImGui::EndTabItem();
      }
      if (WSBeginTabItemWithSel("LFO 3", 6, mModEnvOrLFOTabSelHelper))
      {
        LFO("LFO 3", (int)M7::GigaSynthParamIndices::LFO3Waveform, 2);
        ImGui::EndTabItem();
      }
      if (WSBeginTabItemWithSel("LFO 4", 7, mModEnvOrLFOTabSelHelper))
      {
        LFO("LFO 4", (int)M7::GigaSynthParamIndices::LFO4Waveform, 3);
        ImGui::EndTabItem();
      }
      EndTabBarWithColoredSeparator();
    }

    if (BeginTabBar2("modspectabs", ImGuiTabBarFlags_TabListPopupButton))
    {
      static_assert(M7::gModulationCount == 18, "update this yo");
      for (int i = 0; i < M7::gModulationCount; ++i)
      {
        ModulationSection(i, *this->pMaj7->mpModulations[i]);
      }
      //ModulationSection(1, this->pMaj7->mModulations[1], (int)M7::ParamIndices::Mod2Enabled);
      //ModulationSection(2, this->pMaj7->mModulations[2], (int)M7::ParamIndices::Mod3Enabled);
      //ModulationSection(3, this->pMaj7->mModulations[3], (int)M7::ParamIndices::Mod4Enabled);
      //ModulationSection(4, this->pMaj7->mModulations[4], (int)M7::ParamIndices::Mod5Enabled);
      //ModulationSection(5, this->pMaj7->mModulations[5], (int)M7::ParamIndices::Mod6Enabled);
      //ModulationSection(6, this->pMaj7->mModulations[6], (int)M7::ParamIndices::Mod7Enabled);
      //ModulationSection(7, this->pMaj7->mModulations[7], (int)M7::ParamIndices::Mod8Enabled);
      //ModulationSection(8, this->pMaj7->mModulations[8], (int)M7::ParamIndices::Mod9Enabled);
      //ModulationSection(9, this->pMaj7->mModulations[9], (int)M7::ParamIndices::Mod10Enabled);
      //ModulationSection(10, this->pMaj7->mModulations[10], (int)M7::ParamIndices::Mod11Enabled);
      //ModulationSection(11, this->pMaj7->mModulations[11], (int)M7::ParamIndices::Mod12Enabled);
      //ModulationSection(12, this->pMaj7->mModulations[12], (int)M7::ParamIndices::Mod13Enabled);
      //ModulationSection(13, this->pMaj7->mModulations[13], (int)M7::ParamIndices::Mod14Enabled);
      //ModulationSection(14, this->pMaj7->mModulations[14], (int)M7::ParamIndices::Mod15Enabled);
      //ModulationSection(15, this->pMaj7->mModulations[15], (int)M7::ParamIndices::Mod16Enabled);
      //ModulationSection(16, this->pMaj7->mModulations[16], (int)M7::ParamIndices::Mod17Enabled);
      //ModulationSection(17, this->pMaj7->mModulations[17], (int)M7::ParamIndices::Mod18Enabled);

      EndTabBarWithColoredSeparator();
    }

    if (BeginTabBar2("macroknobs", ImGuiTabBarFlags_None))
    {
      if (WSBeginTabItem("Macros"))
      {
        static_assert(M7::gMacroCount == 4, "update this yo");
        Maj7ImGuiParamMacro(0);
        ImGui::SameLine();
        Maj7ImGuiParamMacro(1);
        ImGui::SameLine();
        Maj7ImGuiParamMacro(2);
        ImGui::SameLine();
        Maj7ImGuiParamMacro(3);
        ImGui::EndTabItem();
      }
      EndTabBarWithColoredSeparator();
    }

    if (mShowingInspector)
    {
      ImGui::SeparatorText("Voice inspector");

      for (size_t i = 0; i < pMaj7->mVoices.Size(); ++i)
      {
        auto pv = (M7::Maj7::Maj7Voice*)pMaj7->mVoices[i];
        char txt[200];
        //if (i & 0x1) ImGui::SameLine();
        auto color = ImColor::HSV(0, 0, .3f);
        if (pv->IsPlaying())
        {
          auto& ns = pMaj7->mMonoNoteStates[pv->mNoteInfo.MidiNoteValue];
          std::sprintf(txt, "%d u:%d", (int)i, pv->mUnisonVoice);
          //std::sprintf(txt, "%d u:%d %d %c%c #%d", (int)i, pv->mUnisonVoice, ns.MidiNoteValue, ns.mIsPhysicallyHeld ? 'P' : ' ', ns.mIsMusicallyHeld ? 'M' : ' ', ns.mSequence);
          color = ImColor::HSV(2 / 7.0f, .8f, .7f);
        }
        else
        {
          std::sprintf(txt, "%d:off", (int)i);
        }
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)color);
        ImGui::Button(txt);
        ImGui::PopStyleColor(3);
        static_assert(M7::gOscillatorCount == 4, "osc count");
        ImGui::SameLine();
        ImGui::ProgressBar(::fabsf(pv->mpOscillatorNodes[0]->GetLastSample()), ImVec2{50, 0}, "Osc1");
        ImGui::SameLine();
        ImGui::ProgressBar(::fabsf(pv->mpOscillatorNodes[1]->GetLastSample()), ImVec2{50, 0}, "Osc2");
        ImGui::SameLine();
        ImGui::ProgressBar(::fabsf(pv->mpOscillatorNodes[2]->GetLastSample()), ImVec2{50, 0}, "Osc3");
        ImGui::SameLine();
        ImGui::ProgressBar(::fabsf(pv->mpOscillatorNodes[3]->GetLastSample()), ImVec2{50, 0}, "Osc4");

        if (mShowingModulationInspector)
        {
          //ImGui::SeparatorText("Modulation Inspector");
          for (size_t idest = 0; idest < std::size(pv->mModMatrix.mDestValues); ++idest)
          {
            char sz[10];
            sprintf_s(sz, "%d", (int)idest);
            if ((idest & 15) == 0)
            {
            }
            else
            {
              ImGui::SameLine();
            }
            ImGui::ProgressBar(::fabsf(pv->mModMatrix.mDestValues[idest]), ImVec2{50, 0}, sz);
          }
        }
      }
    }
  }

  ImGuiKnobs::ModInfo GetModInfo(M7::ModDestination d)
  {
    return mRenderContext.mModDestValues[(int)d];
  }

  void Oscillator(const char* labelWithID, int enabledParamID, int oscID, int ampEnvModDestBase)
  {
    float enabledBacking = GetEffectX()->getParameter(enabledParamID);
    M7::ParamAccessor pa{&enabledBacking, 0};
    //M7::BoolParam bp{ enabledBacking };
    //bp.SetRawParamValue();
    ColorMod& cm = pa.GetBoolValue(0) ? mOscColors : mOscDisabledColors;
    auto token = cm.Push();

    auto lGetModInfo = [&](M7::OscModParamIndexOffsets x)
    {
      return GetModInfo((M7::ModDestination)((int)pMaj7->mpOscillatorDevices[oscID]->mModDestBaseID + (int)x));
    };

    if (mpMaj7VST->mSelectedSource != 0)
    {
      MulDiv(1, 1, 1);
    }

    //sth.GetImGuiFlags(oscID, &mpMaj7VST->mSelectedSource)
    //if (WSBeginTabItem(labelWithID, 0, 0)) {
    if (WSBeginTabItemWithSel(labelWithID, oscID, mSourceTabSelHelper))
    {
      //WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::Enabled, "Enabled");
      Maj7ImGuiBoolParamToggleButton2(enabledParamID + (int)M7::OscParamIndexOffsets::Enabled, "On ", "Off");
      //Maj7ImGuiParamBoolToggleButton(enabledParamID + (int)M7::OscParamIndexOffsets::Enabled, "Enabled");
      ImGui::SameLine();
      Maj7ImGuiParamVolume(enabledParamID + (int)M7::OscParamIndexOffsets::Volume,
                           "Volume",
                           M7::gUnityVolumeCfg,
                           0,
                           lGetModInfo(M7::OscModParamIndexOffsets::Volume));

      ImGui::SameLine();
      WaveformParam(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform,
                    enabledParamID + (int)M7::OscParamIndexOffsets::WaveshapeA,
                    enabledParamID + (int)M7::OscParamIndexOffsets::WaveshapeB,
                    enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset,
                    nullptr);

      M7::QuickParam waveformParam{
          GetEffectX()->getParameter((VstInt32)(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform))};
      auto waveformUiStyle = GetWaveformUiStyle(waveformParam.GetEnumValue<M7::OscillatorWaveform>());

      ImGui::SameLine();
      Maj7ImGuiParamFloat01(enabledParamID + (int)M7::OscParamIndexOffsets::WaveshapeA,
                            waveformUiStyle.shapeALabel,
                            waveformUiStyle.defaultWaveshapeA,
                            waveformUiStyle.defaultWaveshapeA,
                            0,
                            lGetModInfo(M7::OscModParamIndexOffsets::WaveshapeA));
      ImGui::SameLine();
      Maj7ImGuiParamFloat01(enabledParamID + (int)M7::OscParamIndexOffsets::WaveshapeB,
                            waveformUiStyle.shapeBLabel,
                            waveformUiStyle.defaultWaveshapeB,
                            waveformUiStyle.defaultWaveshapeB,
                            0,
                            lGetModInfo(M7::OscModParamIndexOffsets::WaveshapeB));
      ImGui::SameLine();
      Maj7ImGuiParamVolume(enabledParamID + (int)M7::OscParamIndexOffsets::CompensationGain,
                           "Comp.Vol",
                           M7::gVolumeCfg12db,
                           0,
                           {});

      ImGui::SameLine(0, 40);
      Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParam,
                              enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT,
                              "Freq",
                              M7::gSourceFreqConfig,
                              M7::gFreqParamKTUnity,
                              lGetModInfo(M7::OscModParamIndexOffsets::FrequencyParam));
      ImGui::SameLine();
      Maj7ImGuiParamScaledFloat(
          enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "KT", 0, 1, 1, 1, 0, {});
      ImGui::SameLine();
      Maj7ImGuiParamInt(
          enabledParamID + (int)M7::OscParamIndexOffsets::PitchSemis, "Transp", M7::gSourcePitchSemisRange, 0, 0);
      ImGui::SameLine();
      Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PitchFine,
                             "FineTune",
                             0,
                             0,
                             lGetModInfo(M7::OscModParamIndexOffsets::PitchFine));
      ImGui::SameLine();
      Maj7ImGuiParamScaledFloat(
          enabledParamID + (int)M7::OscParamIndexOffsets::FreqMul, "FreqMul", 0, M7::gFrequencyMulMax, 1, 0, 0, {});

      ImGui::SameLine(0, 40);
      Maj7ImGuiBoolParamToggleButton(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseRestart, "Phase\r\nRst");

      ImGui::SameLine();
      Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset,
                             "Phase",
                             0,
                             0,
                             lGetModInfo(M7::OscModParamIndexOffsets::Phase));
      //ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::SyncEnable, "Sync");
      ImGui::SameLine(0, 60);
      Maj7ImGuiBoolParamToggleButton(enabledParamID + (int)M7::OscParamIndexOffsets::SyncEnable, "Sync");
      ImGui::SameLine();
      Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequency,
                              enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT,
                              "SyncFreq",
                              M7::gSyncFreqConfig,
                              M7::gFreqParamKTUnity,
                              lGetModInfo(M7::OscModParamIndexOffsets::SyncFrequency));
      ImGui::SameLine();
      Maj7ImGuiParamScaledFloat(
          enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncKT", 0, 1, 1, 1, 0, {});

      ImGui::SameLine(0, 60);
      Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::Pan,
                             "Pan",
                             0,
                             0,
                             lGetModInfo(M7::OscModParamIndexOffsets::Pan));

      static_assert(M7::gOscillatorCount == 4, "osc count");
      M7::GigaSynthParamIndices ampEnvSources[M7::gOscillatorCount] = {
          M7::GigaSynthParamIndices::Osc1AmpEnvDelayTime,
          M7::GigaSynthParamIndices::Osc2AmpEnvDelayTime,
          M7::GigaSynthParamIndices::Osc3AmpEnvDelayTime,
          M7::GigaSynthParamIndices::Osc4AmpEnvDelayTime,
      };
      auto ampEnvSource = ampEnvSources[oscID];  // ampSourceParam.GetIntValue()];

      ImGui::BeginGroup();
      Maj7ImGuiParamMidiNote(enabledParamID + (int)M7::OscParamIndexOffsets::KeyRangeMin, "KeyRangeMin", 0, 0);
      Maj7ImGuiParamMidiNote(enabledParamID + (int)M7::OscParamIndexOffsets::KeyRangeMax, "KeyRangeMax", 127, 127);
      ImGui::EndGroup();

      ImGui::SameLine();
      {
        //ColorMod::Token ampEnvColorModToken{ (ampSourceParam.GetIntValue() != oscID) ? mPinkColors.mNewColors : nullptr };
        Envelope("Amplitude Envelope", (int)ampEnvSource, ampEnvModDestBase);
      }
      ImGui::EndTabItem();
    }
  }

  void LFO(const char* labelWithID, int waveformParamID, int ilfo)
  {
    ImGui::PushID(labelWithID);

    auto lGetModInfo = [&](M7::LFOModParamIndexOffsets x)
    {
      return GetModInfo((M7::ModDestination)((int)pMaj7->mpLFOs[ilfo]->mDevice.mModDestBaseID + (int)x));
    };

    float phaseCursor = (float)(this->pMaj7->mpLFOs[ilfo]->mPhase.GetPhase01());

    TIME_BASIS_CAPTIONS(timeBasisCaptions);

    WaveformParam(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform,
                  waveformParamID + (int)M7::LFOParamIndexOffsets::WaveshapeA,
                  waveformParamID + (int)M7::LFOParamIndexOffsets::WaveshapeB,
                  waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset,
                  &phaseCursor);

    M7::QuickParam waveformParam{
        GetEffectX()->getParameter((VstInt32)(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform))};
    auto waveformUiStyle = GetWaveformUiStyle(waveformParam.GetEnumValue<M7::OscillatorWaveform>());

    ImGui::SameLine();
    Maj7ImGuiParamFloat01(waveformParamID + (int)M7::LFOParamIndexOffsets::WaveshapeA,
                          waveformUiStyle.shapeALabel,
                          waveformUiStyle.defaultWaveshapeA,
                          waveformUiStyle.defaultWaveshapeA,
                          0,
                          lGetModInfo(M7::LFOModParamIndexOffsets::WaveshapeA));
    ImGui::SameLine();
    Maj7ImGuiParamFloat01(waveformParamID + (int)M7::LFOParamIndexOffsets::WaveshapeB,
                          waveformUiStyle.shapeBLabel,
                          waveformUiStyle.defaultWaveshapeB,
                          waveformUiStyle.defaultWaveshapeB,
                          0,
                          lGetModInfo(M7::LFOModParamIndexOffsets::WaveshapeB));
    ImGui::SameLine();


    ////static constexpr char const* const voiceModeCaptions[] = {"Poly", "Mono"};
    ////ImGui::SameLine(0, 60);
    //Maj7ImGuiParamEnumList<M7::TimeBasis>(waveformParamID + (int)M7::LFOParamIndexOffsets::TimeBasis,
    //                                                 "TimeBasis",
    //                                                 (int)M7::TimeBasis::Count,
    //                                                 M7::TimeBasis::Frequency,
    //                                                 timeBasisCaptions);

    //M7::QuickParam timeBasisParam{
    //    GetEffectX()->getParameter((VstInt32)(waveformParamID + (int)M7::LFOParamIndexOffsets::TimeBasis))};
    //    const auto timeBasis = timeBasisParam.GetEnumValue<M7::TimeBasis>();

    //    switch (timeBasis)
    //    {
    //      case M7::TimeBasis::Beats:
    //        ImGui::SameLine();
    //        Maj7ImGuiParamInt(waveformParamID + (int)M7::LFOParamIndexOffsets::BeatNumerator, "BeatNum", M7::gLFOBeatNumeratorCfg, 3, 0);
    //        ImGui::SameLine();
    //        Maj7ImGuiParamInt(waveformParamID + (int)M7::LFOParamIndexOffsets::BeatDenominator, "Denom", M7::gLFOBeatDenominatorCfg, 1, 1);
    //        ImGui::SameLine();
    //        Maj7ImGuiParamFloatN11(waveformParamID + (int)M7::LFOParamIndexOffsets::DurationEighthsFine, "(fine)##left", 0, 0, lGetModInfo(M7::LFOModParamIndexOffsets::DurationEighthsFine));
    //        break;
    //      case M7::TimeBasis::Frequency:
            ImGui::SameLine();
            Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::FrequencyParam,
                            -1,
                            "Freq",
                            M7::gLFOFreqConfig,
                            0.4f,
                            lGetModInfo(M7::LFOModParamIndexOffsets::FrequencyParam));
                            ImGui::SameLine();
        //    break;
        //  case M7::TimeBasis::Time:
        //    ImGui::SameLine();
        //    // Maj7ImGuiBipolarPowCurvedParam(waveformParamID + (int)M7::LFOParamIndexOffsets::DurationMS,
        //    //   "(ms)##left", M7::gEnvTimeCfg, 0, lGetModInfo(M7::LFOModParamIndexOffsets::DurationMilliseconds));
        //    Maj7ImGuiPowCurvedParam(waveformParamID + (int)M7::LFOParamIndexOffsets::DurationMS,
        //                    "Millisecs",
        //                    M7::gLFOTimeCfg,
        //                    250,
        //                    lGetModInfo(M7::LFOModParamIndexOffsets::DurationMilliseconds));
        //    break;
        //  default:
        //    ImGui::Text("Unknown time basis!");
        //    break;
        //}



                            ImGui::SameLine(0, 30);
    Maj7ImGuiParamFloatN11(waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset,
                           "Phase",
                           0,
                           0,
                           lGetModInfo(M7::LFOModParamIndexOffsets::Phase));
    //ImGui::SameLine(); WSImGuiParamCheckbox(waveformParamID + (int)M7::LFOParamIndexOffsets::Restart, "Restart");
    ImGui::SameLine();
    Maj7ImGuiBoolParamToggleButton(waveformParamID + (int)M7::LFOParamIndexOffsets::Restart, "Restart");

    ImGui::SameLine(0, 60);
    Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::Sharpness,
                            -1,
                            "Sharpness",
                            M7::gLFOLPFreqConfig,
                            0.5f,
                            lGetModInfo(M7::LFOModParamIndexOffsets::Sharpness));

    ImGui::PopID();
  }

  void Envelope(const char* labelWithID, int delayTimeParamID, int modDestBaseID)
  {
    auto lGetModInfo = [&](M7::EnvModParamIndexOffsets x)
    {
      return GetModInfo((M7::ModDestination)(modDestBaseID + (int)x));
    };

    ImGui::PushID(labelWithID);
    Maj7ImGuiPowCurvedParam(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DelayTime,
                            "Delay",
                            M7::gEnvTimeCfg,
                            0,
                            lGetModInfo(M7::EnvModParamIndexOffsets::DelayTime));
    ImGui::SameLine();
    Maj7ImGuiPowCurvedParam(delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackTime,
                            "Attack",
                            M7::gEnvTimeCfg,
                            0,
                            lGetModInfo(M7::EnvModParamIndexOffsets::AttackTime));
    ImGui::SameLine();
    Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackCurve,
                        "Curve##attack",
                        0,
                        M7CurveRenderStyle::Rising,
                        lGetModInfo(M7::EnvModParamIndexOffsets::AttackCurve));
    ImGui::SameLine();
    Maj7ImGuiPowCurvedParam(delayTimeParamID + (int)M7::EnvParamIndexOffsets::HoldTime,
                            "Hold",
                            M7::gEnvTimeCfg,
                            0,
                            lGetModInfo(M7::EnvModParamIndexOffsets::HoldTime));
    ImGui::SameLine();
    Maj7ImGuiPowCurvedParam(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayTime,
                            "Decay",
                            M7::gEnvTimeCfg,
                            .4f,
                            lGetModInfo(M7::EnvModParamIndexOffsets::DecayTime));
    ImGui::SameLine();
    Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayCurve,
                        "Curve##Decay",
                        0,
                        M7CurveRenderStyle::Falling,
                        lGetModInfo(M7::EnvModParamIndexOffsets::DecayCurve));
    ImGui::SameLine();
    Maj7ImGuiParamFloat01(delayTimeParamID + (int)M7::EnvParamIndexOffsets::SustainLevel,
                          "Sustain",
                          0.4f,
                          0,
                          0,
                          lGetModInfo(M7::EnvModParamIndexOffsets::SustainLevel));
    ImGui::SameLine();
    Maj7ImGuiPowCurvedParam(delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseTime,
                            "Release",
                            M7::gEnvTimeCfg,
                            0,
                            lGetModInfo(M7::EnvModParamIndexOffsets::ReleaseTime));
    ImGui::SameLine();
    Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseCurve,
                        "Curve##Release",
                        0,
                        M7CurveRenderStyle::Falling,
                        lGetModInfo(M7::EnvModParamIndexOffsets::ReleaseCurve));
    ImGui::SameLine();
    ImGui::BeginGroup();
    Maj7ImGuiBoolParamToggleButton(delayTimeParamID + (int)M7::EnvParamIndexOffsets::LegatoRestart, "Leg.Restart");
    //WSImGuiParamCheckbox(delayTimeParamID + (int)M7::EnvParamIndexOffsets::LegatoRestart, "Leg.Restart");

    ImGui::Text("OneShot");

    M7::QuickParam qp{GetEffectX()->getParameter(delayTimeParamID + (int)M7::EnvParamIndexOffsets::Mode)};

    //float backing = GetEffectX()->getParameter(delayTimeParamID + (int)M7::EnvParamIndexOffsets::Mode);
    //M7::EnumParam<M7::EnvelopeMode> modeParam{ backing, M7::EnvelopeMode::Count };
    //M7::ParamAccessor pa{ &backing, 0 };
    bool boneshot = qp.GetEnumValue<M7::EnvelopeMode>() == M7::EnvelopeMode::OneShot;
    bool r = ImGui::Checkbox("##cb", &boneshot);
    if (r)
    {
      qp.SetEnumValue(boneshot ? M7::EnvelopeMode::OneShot : M7::EnvelopeMode::Sustain);
      GetEffectX()->setParameterAutomated(delayTimeParamID + (int)M7::EnvParamIndexOffsets::Mode, qp.GetRawValue());
    }

    ImGui::EndGroup();

    ImGui::SameLine();
    Maj7ImGuiEnvelopeGraphic("graph", delayTimeParamID);
    ImGui::PopID();
  }


  std::string GetModulationName(M7::ModulationSpec& spec, int imod)
  {
    char ret[200];

    M7::QuickParam enabledParam{
        GetEffectX()->getParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Enabled))};
    //M7::BoolParam mEnabled {enabledBacking};
    if (!enabledParam.GetBoolValue())
    {
      sprintf_s(ret, "Mod %d###mod%d", imod, imod);
      return ret;
    }

    M7::QuickParam srcParam{GetEffectX()->getParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Source))};
    //float srcBacking = GetEffectX()->getParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Source));
    //M7::EnumParam<M7::ModSource> mSource{ srcBacking, M7::ModSource::Count };
    auto src = srcParam.GetEnumValue<M7::ModSource>();
    if (src == M7::ModSource::None)
    {
      sprintf_s(ret, "Mod %d###mod%d", imod, imod);
      return ret;
    }

    M7::QuickParam destParam{
        GetEffectX()->getParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Destination1))};
    //float destBacking = GetEffectX()->getParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Destination1));
    //M7::EnumParam<M7::ModDestination> mDestination{ destBacking, M7::ModDestination::Count };
    //auto dest = mDestination.GetEnumValue();
    auto dest = destParam.GetEnumValue<M7::ModDestination>();
    if (dest == M7::ModDestination::None)
    {
      sprintf_s(ret, "Mod %d###mod%d", imod, imod);
      return ret;
    }

    MODSOURCE_SHORT_CAPTIONS(srcCaptions);

    if (spec.mType == M7::ModulationSpecType::SourceAmp)
    {
      if ((int)src < 0 || (int)src >= (int)M7::ModSource::Count)
      {
        sprintf_s(ret, "(unknown source %d)###mod%d", (int)src, imod);
        return ret;
      }

      sprintf_s(ret, "%s###mod%d", srcCaptions[(int)src], imod);
      return ret;
    }

    MODDEST_SHORT_CAPTIONS(destCaptions);

    const char* srcCaption = "(unknown src)";
    if ((int)src >= 0 && (int)src < (int)M7::ModSource::Count)
    {
      srcCaption = srcCaptions[(int)src];
    }

    const char* destCaption = "(unknown dest)";
    if ((int)dest >= 0 && (int)dest < (int)M7::ModDestination::Count)
    {
      destCaption = destCaptions[(int)dest];
    }

    sprintf_s(ret, "%s > %s###mod%d", srcCaption, destCaption, imod);
    return ret;
  }

  struct ModMeterStyle
  {
    ImColor background;
    ImColor backgroundOob;
    ImColor foreground;
    ImColor foregorundOob;
    ImColor foregroundTick;
    ImColor boundaryIndicator;
    ImColor zeroTick;
    ImVec2 meterSize;
    float gHalfTickWidth;
    float gTickHeight;

    ModMeterStyle Modification(float saturationMul, float valMul, float width, float height)
    {
      ModMeterStyle ret{
          ColorMod::GetModifiedColor(background, 0, saturationMul, valMul),
          ColorMod::GetModifiedColor(backgroundOob, 0, saturationMul, valMul),
          ColorMod::GetModifiedColor(foreground, 0, saturationMul, valMul),
          ColorMod::GetModifiedColor(foregorundOob, 0, saturationMul, valMul),
          ColorMod::GetModifiedColor(foregroundTick, 0, saturationMul, valMul),
          ColorMod::GetModifiedColor(boundaryIndicator, 0, saturationMul, valMul),
          ColorMod::GetModifiedColor(zeroTick, 0, saturationMul, valMul),
          meterSize,
          gHalfTickWidth,
          gTickHeight,
      };
      ret.meterSize.x = width;
      ret.meterSize.y = height;
      return ret;
    }
  };

  ModMeterStyle mBaseModMeterStyle{
      ColorFromHTML("0d350d"),
      ColorFromHTML("404040"),
      ColorFromHTML("228822"),
      ColorFromHTML("882222"),
      ColorFromHTML("40ff40"),
      ColorFromHTML("854ecb"),
      ColorFromHTML("000000"),
      {175, 9},
      0.5f,
      2.0f,
  };

  ModMeterStyle mIntermediateLargeModMeterStyle = mBaseModMeterStyle.Modification(1, 1, 175, 9);
  ModMeterStyle mIntermediateSmallModMeterStyle = mBaseModMeterStyle.Modification(.3f, 0.85f, 175, 6);

  ModMeterStyle mIntermediateSmallModMeterStyleDisabled = mBaseModMeterStyle.Modification(0, 0.7f, 175, 6);
  ModMeterStyle mIntermediateLargeModMeterStyleDisabled = mBaseModMeterStyle.Modification(0, 0.7f, 175, 9);

  ModMeterStyle mPrimaryModMeterStyle = mBaseModMeterStyle.Modification(1, 1, 250, 10);
  ModMeterStyle mPrimaryModMeterStyleDisabled = mBaseModMeterStyle.Modification(0, 0.7f, 250, 10);

  void AddRect(float x1, float x2, float y1, float y2, const ImColor& color)
  {
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2{std::min(x1, x2), std::min(y1, y2)},
                                              ImVec2{std::max(x1, x2), std::max(y1, y2)},
                                              color);
  }

  // rangemin/max are the window being shown.
  // boundmin/max are the area within the window which are colored green.
  void ModMeter_Horiz(M7::ModulationSpec& spec,
                      ImVec2 orig,
                      ImVec2 size,
                      float windowMin,
                      float windowMax,
                      float srcBound1,
                      float srcBound2,
                      float highlightBound1,
                      float highlightBound2,
                      float x,
                      const ModMeterStyle& style)
  {
    auto ValToX = [&](float x_)
    {
      // map x from [rangemin,rangemax] to rect.
      return M7::math::map_clamped(x_, windowMin, windowMax, orig.x, orig.x + size.x);
    };

    auto* dl = ImGui::GetWindowDrawList();

    // background oob
    dl->AddRectFilled(orig, orig + size, style.backgroundOob);

    // background, in src bound
    float xSrcBound1 = ValToX(srcBound1);
    float xSrcBound2 = ValToX(srcBound2);
    auto srcBoundIntersection = Intersect(xSrcBound1, xSrcBound2, orig.x, orig.x + size.x);
    if (std::get<0>(srcBoundIntersection))
    {
      AddRect(std::get<1>(srcBoundIntersection),
              std::get<2>(srcBoundIntersection),
              orig.y,
              orig.y + size.y,
              style.background);
    }

    // oob area
    float xzero = ValToX(0);
    float xval = ValToX(x);
    AddRect(xzero, xval, orig.y, orig.y + size.y, style.foregorundOob);

    // in-bound highlight area
    float xHighlightBound1 = ValToX(highlightBound1);
    float xHighlightBound2 = ValToX(highlightBound2);
    auto highlightRangeIntersection = Intersect(xHighlightBound1, xHighlightBound2, xzero, xval);
    if (std::get<0>(highlightRangeIntersection))
    {
      AddRect(std::get<1>(highlightRangeIntersection),
              std::get<2>(highlightRangeIntersection),
              orig.y,
              orig.y + size.y,
              style.foreground);
    }

    // foreground tick
    AddRect(xval - style.gHalfTickWidth, xval + style.gHalfTickWidth, orig.y, orig.y + size.y, style.foregroundTick);

    // zero tick
    AddRect(xzero - style.gHalfTickWidth, xzero + style.gHalfTickWidth, orig.y, orig.y + size.y, style.zeroTick);

    // .5 ticks
    float windowBoundLow = std::min(windowMax, windowMin);
    float windowBoundHigh = std::max(windowMax, windowMin);
    windowBoundLow = floorf(windowBoundLow * 2) * 0.5f;  // round down to 0.5.
    for (float f = windowBoundLow; f < windowBoundHigh; f += 0.5f)
    {
      float xtick = ValToX(f);
      if (xtick <= (orig.x + 4.0f))
        continue;  // don't show ticks that are close to the edge; looks ugly.
      if (xtick >= (orig.x + size.x - 4.0f))
        continue;  // don't show ticks that are close to the edge; looks ugly.
      AddRect(xtick - style.gHalfTickWidth,
              xtick + style.gHalfTickWidth,
              orig.y,
              orig.y + style.gTickHeight,
              style.zeroTick);
      AddRect(xtick - style.gHalfTickWidth,
              xtick + style.gHalfTickWidth,
              orig.y + size.y - style.gTickHeight,
              orig.y + size.y,
              style.zeroTick);
    }

    ImGui::Dummy(size);
  }

  // draws:
  // - source value, colored by whether it falls in the source range
  // - range arrows
  // - result value scaled and curved -1,1
  // returns the resulting value.
  float ModMeter(bool visible,
                 M7::ModulationSpec& spec,
                 M7::ModSource modSource,
                 M7::ModParamIndexOffsets curveParam,
                 M7::ModParamIndexOffsets rangeMinParam,
                 M7::ModParamIndexOffsets rangeMaxParam,
                 bool isTargetN11,
                 bool isEnabled)
  {
    float srcVal = mRenderContext.mModSourceValues[(int)modSource];
    float rangeMin = spec.mParams.GetScaledRealValue(rangeMinParam, -3, 3, 0);
    float rangeMax = spec.mParams.GetScaledRealValue(rangeMaxParam, -3, 3, 0);
    float resultingValue =
        mRenderContext.mpModMatrix->MapValue(spec, modSource, curveParam, rangeMinParam, rangeMaxParam, isTargetN11);
    if (!visible)
    {
      return resultingValue;
    }

    auto& smallStyle = *(isEnabled ? &mIntermediateSmallModMeterStyle : &mIntermediateSmallModMeterStyleDisabled);
    auto& largeStyle = *(isEnabled ? &mIntermediateLargeModMeterStyle : &mIntermediateLargeModMeterStyleDisabled);

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    float sourceWindowMin = std::min(std::min(-1.0f, rangeMin), rangeMax);
    float sourceWindowMax = std::max(std::max(1.0f, rangeMin), rangeMax);
    ModMeter_Horiz(spec,
                   window->DC.CursorPos,
                   smallStyle.meterSize,
                   sourceWindowMin,
                   sourceWindowMax,
                   -1,
                   1,
                   rangeMin,
                   rangeMax,
                   srcVal,
                   smallStyle);
    TooltipF("Raw input signal");

    // get highlight area of the total source window
    float highlightWindowMin =
        M7::math::map_clamped(rangeMin, sourceWindowMin, sourceWindowMax, 0, smallStyle.meterSize.x);
    float highlightWindowMax =
        M7::math::map_clamped(rangeMax, sourceWindowMin, sourceWindowMax, 0, smallStyle.meterSize.x);

    // little handle indicators
    static constexpr ImVec2 gHandleSize{5, 5};
    static constexpr float gHandleThickness = 2;
    static constexpr float gHandleMarginX = 2;

    auto pos = window->DC.CursorPos;
    ModMeter_Horiz(spec,
                   {window->DC.CursorPos.x + highlightWindowMin, window->DC.CursorPos.y},
                   {highlightWindowMax - highlightWindowMin, smallStyle.meterSize.y},
                   rangeMin,
                   rangeMax,
                   rangeMin,
                   rangeMax,
                   rangeMin,
                   rangeMax,
                   srcVal,
                   smallStyle);
    TooltipF("Selected range to use for modulation");

    AddRect(pos.x + highlightWindowMin - gHandleSize.x - gHandleMarginX,
            pos.x + highlightWindowMin - gHandleMarginX,
            pos.y,
            pos.y + gHandleThickness,
            smallStyle.boundaryIndicator);

    AddRect(pos.x + highlightWindowMin - gHandleThickness - gHandleMarginX,
            pos.x + highlightWindowMin - gHandleMarginX,
            pos.y,
            pos.y + gHandleSize.y,
            smallStyle.boundaryIndicator);

    AddRect(  // TOP
        pos.x + highlightWindowMax + gHandleMarginX,
        pos.x + highlightWindowMax + gHandleMarginX + gHandleSize.x,
        pos.y,
        pos.y + gHandleThickness,
        smallStyle.boundaryIndicator);

    AddRect(  // BOTTOM
        pos.x + highlightWindowMax + gHandleMarginX,
        pos.x + highlightWindowMax + gHandleMarginX + gHandleThickness,
        pos.y,
        pos.y + gHandleSize.y,
        smallStyle.boundaryIndicator);

    // show output value.
    ModMeter_Horiz(spec,
                   window->DC.CursorPos,
                   largeStyle.meterSize,
                   isTargetN11 ? -1.0f : 0.0f,
                   1,
                   -1,
                   1,
                   -1,
                   1,
                   resultingValue,
                   largeStyle);
    TooltipF("Value after selected range and curve applied");

    return resultingValue;
  }


  void ModulationSection(int imod, M7::ModulationSpec& spec)
  {
    bool isLocked = spec.mType != M7::ModulationSpecType::General;

    static constexpr char const* const modSourceCaptions[] = MOD_SOURCE_CAPTIONS;
    std::string modDestinationCaptions[(size_t)M7::ModDestination::Count] = MOD_DEST_CAPTIONS;
    char const* modDestinationCaptionsCstr[(size_t)M7::ModDestination::Count];

    for (size_t i = 0; i < (size_t)M7::ModDestination::Count; ++i)
    {
      modDestinationCaptionsCstr[i] = modDestinationCaptions[i].c_str();
    }

    bool isEnabled = spec.mParams.GetBoolValue(M7::ModParamIndexOffsets::Enabled);
    float modVal = 0;

    ColorMod& cm = isEnabled ? (isLocked ? mLockedModulationsColors : mModulationsColors) : mModulationDisabledColors;
    auto token = cm.Push();

    if (WSBeginTabItemWithSel(GetModulationName(spec, imod).c_str(), imod, mModulationTabSelHelper))
    {
      ImGui::BeginDisabled(isLocked);
      int enabledParamID = spec.mParams.mBaseParamID;
      //WSImGuiParamCheckbox(spec.mParams.mBaseParamID + (int)M7::ModParamIndexOffsets::Enabled, "Enabled");
      Maj7ImGuiBoolParamToggleButton2(spec.mParams.mBaseParamID + (int)M7::ModParamIndexOffsets::Enabled, "On ", "Off");
      ImGui::EndDisabled();
      ImGui::SameLine();
      {
        ImGui::BeginGroup();
        ImGui::PushID("main");

        Maj7ImGuiParamEnumCombo(enabledParamID + (int)M7::ModParamIndexOffsets::Source,
                                "Source",
                                (int)M7::ModSource::Count,
                                M7::ModSource::None,
                                modSourceCaptions,
                                180);

        if (mpMaj7VST->mShowAdvancedModControls[imod])
        {
          Maj7ImGuiParamScaledFloat(
              enabledParamID + (int)M7::ModParamIndexOffsets::SrcRangeMin, "RangeMin", -3, 3, -1, -1, 30, {});
          ImGui::SameLine();
          Maj7ImGuiParamScaledFloat(
              enabledParamID + (int)M7::ModParamIndexOffsets::SrcRangeMax, "Max", -3, 3, 1, 1, 30, {});

          ImGui::SameLine();
          Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Curve,
                              "Curve",
                              0,
                              M7CurveRenderStyle::Rising,
                              {});

          modVal = ModMeter(true,
                            spec,
                            spec.mSource,
                            M7::ModParamIndexOffsets::Curve,
                            M7::ModParamIndexOffsets::SrcRangeMin,
                            M7::ModParamIndexOffsets::SrcRangeMax,
                            true,
                            isEnabled);

          if (ImGui::SmallButton("Reset"))
          {
            spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMin, -3, 3, -1);
            spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMax, -3, 3, 1);
          }
          ImGui::SameLine();
          if (ImGui::SmallButton("To pos"))
          {
            spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMin, -3, 3, -3);
            spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMax, -3, 3, 1);
          }
          ImGui::SameLine();
          if (ImGui::SmallButton("To bip."))
          {
            spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMin, -3, 3, 0);
            spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMax, -3, 3, 1);
          }
          if (ImGui::SmallButton("hide advanced"))
          {
            mpMaj7VST->mShowAdvancedModControls[imod] = false;
          }
        }
        else
        {
          modVal = ModMeter(false,
                            spec,
                            spec.mSource,
                            M7::ModParamIndexOffsets::Curve,
                            M7::ModParamIndexOffsets::SrcRangeMin,
                            M7::ModParamIndexOffsets::SrcRangeMax,
                            true,
                            isEnabled);
          if (ImGui::SmallButton("show advanced"))
          {
            mpMaj7VST->mShowAdvancedModControls[imod] = true;
          }
        }

        ImGui::PopID();
        ImGui::EndGroup();
      }
      ImGui::SameLine();
      ImGui::BeginDisabled(isLocked);
      {
        ImGui::BeginGroup();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});

        Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination1,
                                "Dest##1",
                                (int)M7::ModDestination::Count,
                                M7::ModDestination::None,
                                modDestinationCaptionsCstr,
                                180);
        ImGui::SameLine();
        Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale1, "###Scale1", 1, 20.0f, {});

        Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination2,
                                "###Dest2",
                                (int)M7::ModDestination::Count,
                                M7::ModDestination::None,
                                modDestinationCaptionsCstr,
                                180);
        ImGui::SameLine();
        Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale2, "###Scale2", 1, 20.0f, {});

        Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination3,
                                "###Dest3",
                                (int)M7::ModDestination::Count,
                                M7::ModDestination::None,
                                modDestinationCaptionsCstr,
                                180);
        ImGui::SameLine();
        Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale3, "###Scale3", 1, 20.0f, {});

        Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination4,
                                "###Dest4",
                                (int)M7::ModDestination::Count,
                                M7::ModDestination::None,
                                modDestinationCaptionsCstr,
                                180);
        ImGui::SameLine();
        Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale4, "###Scale4", 1, 20.0f, {});

        ImGui::PopStyleVar();
        ImGui::EndGroup();
      }
      ImGui::EndDisabled();

      ////ImGui::SameLine(0, 60); WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxEnabled, "Aux Enable");
      //ImGui::SameLine(0, 60);
      //Maj7ImGuiBoolParamToggleButton((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxEnabled,
      //                               "Aux Enable");
      //bool isAuxEnabled = spec.mParams.GetBoolValue(M7::ModParamIndexOffsets::AuxEnabled);

      //{
      //  ColorMod& cmaux = isAuxEnabled ? mNopColors : mModulationDisabledColors;
      //  auto auxToken = cmaux.Push();
      //  ImGui::PushID("aux");
      //  ImGui::SameLine();
      //  {
      //    ImGui::BeginGroup();
      //    Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxSource,
      //                            "Aux Src",
      //                            (int)M7::ModSource::Count,
      //                            M7::ModSource::None,
      //                            modSourceCaptions,
      //                            180);
      //    float auxVal = 0;
      //    if (mpMaj7VST->mShowAdvancedModAuxControls[imod])
      //    {
      //      Maj7ImGuiParamScaledFloat(
      //          enabledParamID + (int)M7::ModParamIndexOffsets::AuxRangeMin, "AuxRangeMin", -3, 3, 0, 0, 30, {});
      //      ImGui::SameLine();
      //      Maj7ImGuiParamScaledFloat(
      //          enabledParamID + (int)M7::ModParamIndexOffsets::AuxRangeMax, "Max", -3, 3, 1, 1, 30, {});

      //      ImGui::SameLine();
      //      Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxCurve,
      //                          "Aux Curve",
      //                          0,
      //                          M7CurveRenderStyle::Rising,
      //                          {});

      //      auxVal = ModMeter(true,
      //                        spec,
      //                        spec.mAuxSource,
      //                        M7::ModParamIndexOffsets::AuxCurve,
      //                        M7::ModParamIndexOffsets::AuxRangeMin,
      //                        M7::ModParamIndexOffsets::AuxRangeMax,
      //                        false,
      //                        isEnabled && isAuxEnabled);

      //      if (ImGui::SmallButton("Reset"))
      //      {
      //        spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMin, -3, 3, 0);
      //        spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMax, -3, 3, 1);
      //      }
      //      ImGui::SameLine();
      //      if (ImGui::SmallButton("bip. to pos"))
      //      {
      //        spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMin, -3, 3, -3);
      //        spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMax, -3, 3, 1);
      //      }
      //      ImGui::SameLine();
      //      if (ImGui::SmallButton("neg"))
      //      {
      //        spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMin, -3, 3, 1);
      //        spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMax, -3, 3, 0);
      //      }

      //      if (ImGui::SmallButton("hide advanced"))
      //      {
      //        mpMaj7VST->mShowAdvancedModAuxControls[imod] = false;
      //      }
      //    }
      //    else
      //    {
      //      auxVal = ModMeter(false,
      //                        spec,
      //                        spec.mAuxSource,
      //                        M7::ModParamIndexOffsets::AuxCurve,
      //                        M7::ModParamIndexOffsets::AuxRangeMin,
      //                        M7::ModParamIndexOffsets::AuxRangeMax,
      //                        false,
      //                        isEnabled && isAuxEnabled);
      //      if (ImGui::SmallButton("show advanced"))
      //      {
      //        mpMaj7VST->mShowAdvancedModAuxControls[imod] = true;
      //      }
      //    }

      //    float auxAtten = spec.mParams.Get01Value(M7::ModParamIndexOffsets::AuxAttenuation);
      //    float auxScale = M7::math::lerp(1, 1.0f - auxAtten, auxVal);
      //    modVal *= auxScale;

      //    ImGui::EndGroup();
      //  }
      //  ImGui::SameLine();
      //  WSImGuiParamKnob(enabledParamID + (int)M7::ModParamIndexOffsets::AuxAttenuation, "Atten");
      //  ImGui::PopID();
      //}

      ImGui::SameLine();
      ImGui::BeginGroup();
      if (ImGui::SmallButton("Copy to..."))
      {
        ImGui::OpenPopup("copymodto");
      }

      if (ImGui::BeginPopup("copymodto"))
      {
        for (int n = 0; n < (int)M7::gModulationCount; n++)
        {
          ImGui::PushID(n);

          char modName[100];
          sprintf_s(modName, "Mod %d", n);

          if (ImGui::Selectable(GetModulationName(*this->pMaj7->mpModulations[n], n).c_str()))
          {
            M7::ModulationSpec& srcSpec = spec;
            M7::ModulationSpec& destSpec = *this->pMaj7->mpModulations[n];
            for (int iparam = 0; iparam < (int)M7::ModParamIndexOffsets::Count; ++iparam)
            {
              float p = GetEffectX()->getParameter(srcSpec.mParams.GetParamIndex(iparam));
              GetEffectX()->setParameter(destSpec.mParams.GetParamIndex(iparam), p);
            }
          }
          ImGui::PopID();
        }
        ImGui::EndPopup();
      }

      ImGui::BeginDisabled(isLocked);
      if (ImGui::SmallButton("Load preset..."))
      {
        ImGui::OpenPopup("loadmodpreset");
      }
      ImGui::EndDisabled();

      if (ImGui::BeginPopup("loadmodpreset"))
      {
        if (ImGui::Selectable("Init"))
        {
          M7::GenerateDefaults(&spec);
          GetEffectX()->setParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Enabled),
                                     0);  // to mark the patch as modified.
        }
        if (ImGui::Selectable("Unisono -> Phase (+osc phase restart)"))
        {
          M7::GenerateDefaults(&spec);
          GetEffectX()->setParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Enabled),
                                     1);  // to mark the patch as modified.
          spec.mParams.SetEnumValue(M7::ModParamIndexOffsets::Source, M7::ModSource::UnisonoVoice);
          spec.mParams.SetEnumValue(M7::ModParamIndexOffsets::Destination1, M7::ModDestination::Osc1Phase);
          spec.mParams.SetEnumValue(M7::ModParamIndexOffsets::Destination2, M7::ModDestination::Osc2Phase);
          spec.mParams.SetEnumValue(M7::ModParamIndexOffsets::Destination3, M7::ModDestination::Osc3Phase);
          spec.mParams.SetEnumValue(M7::ModParamIndexOffsets::Destination4, M7::ModDestination::Osc4Phase);

          spec.mParams.SetN11Value(M7::ModParamIndexOffsets::Scale1, 0.618f);
          spec.mParams.SetN11Value(M7::ModParamIndexOffsets::Scale2, 0.618f);
          spec.mParams.SetN11Value(M7::ModParamIndexOffsets::Scale3, 0.618f);
          spec.mParams.SetN11Value(M7::ModParamIndexOffsets::Scale4, 0.618f);

          pMaj7->mParams.SetBoolValue(M7::GigaSynthParamIndices::Osc1PhaseRestart, true);
          pMaj7->mParams.SetBoolValue(M7::GigaSynthParamIndices::Osc2PhaseRestart, true);
          pMaj7->mParams.SetBoolValue(M7::GigaSynthParamIndices::Osc3PhaseRestart, true);
          pMaj7->mParams.SetBoolValue(M7::GigaSynthParamIndices::Osc4PhaseRestart, true);
        }
        ImGui::EndPopup();
      }

      ImGuiWindow* window = ImGui::GetCurrentWindow();
      ModMeter_Horiz(spec,
                     window->DC.CursorPos,
                     mPrimaryModMeterStyle.meterSize,
                     -1,
                     1,
                     -1,
                     1,
                     -1,
                     1,
                     modVal,
                     isEnabled ? mPrimaryModMeterStyle : mPrimaryModMeterStyleDisabled);
      TooltipF("Modulation output value before it's scaled to the various destinations");

      ImGui::EndGroup();

      ImGui::EndTabItem();
    }
  }


  void AuxEffectTab(const char* labelID)
  {
    auto& filter = pMaj7->mMaj7Voice[0]->mFilter[0];

    ColorMod& cm = filter.mParams.GetBoolValue(M7::FilterParamIndexOffsets::Enabled) ? mAuxLeftColors
                                                                                     : mAuxLeftDisabledColors;
    auto token = cm.Push();

    if (WSBeginTabItemWithSel(labelID, 0, mFilterTabSelHelper))
    {
      auto lGetModInfo = [&](M7::ModDestination x)
      {
        return GetModInfo(x);
      };

      //WSImGuiParamCheckbox(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::Enabled), "Enabled");
      Maj7ImGuiBoolParamToggleButton2(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::Enabled), "On ", "Off");

      ImGui::SameLine(0, 0);

      const int filterResponseParamID = filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::FilterResponse);

      M7::QuickParam filterResponseParam{GetEffectX()->getParameter(filterResponseParamID)};

      M7::FilterResponse selectedResponse = filterResponseParam.GetEnumValue<M7::FilterResponse>();
      const float selectedCutoffHz = filter.mParams.GetFrequency(M7::FilterParamIndexOffsets::Freq, M7::gFilterFreqConfig);
      const float selectedReso01 = filter.mParams.Get01Value(M7::FilterParamIndexOffsets::Q);

      auto applySelection = [&](M7::FilterResponse response)
      {
        M7::QuickParam qp{};
        GetEffectX()->setParameter(filterResponseParamID, qp.SetEnumValue(response));
        selectedResponse = response;
      };
      RenderFilterSelectionWidget(
          labelID,
          9000,
          selectedResponse,
          WaveSabreCore::M7::DoesFilterSupport,
          [](M7::FilterResponse response)
          {
            switch (response)
            {
              case M7::FilterResponse::Peak:
              case M7::FilterResponse::Allpass:
              case M7::FilterResponse::LowShelf:
              case M7::FilterResponse::HighShelf:
                return false;
              default:
                return true;
            }
          },
          applySelection,
          [&](const ImRect& bb)
          {
            auto* dl = ImGui::GetWindowDrawList();
            dl->AddRect(bb.Min, bb.Max, ColorFromHTML("2a2a2a"), 3.0f, 0, 1.5f);
            const auto& filterPreview = GetOrBuildFilterPreviewCache(0,
                                                                      std::max(8, (int)bb.GetWidth() - 8),
                                                                      std::max(8, (int)bb.GetHeight() - 8),
                                                                      selectedResponse,
                                                                      selectedCutoffHz,
                                                                      selectedReso01);
            DrawFilterPreview(bb, filterPreview);
            DrawShadowText(BuildFilterSelectionButtonLabel(selectedResponse),
                           ImVec2(bb.Min.x + 6, bb.Min.y + 3));
          });

      Maj7ImGuiParamFrequency(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::Freq),
                              filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::FreqKT),
                              "Freq##filt",
                              M7::gFilterFreqConfig,
                              M7::gFreqParamKTUnity,
                              lGetModInfo(M7::ModDestination::Filter1Freq));
      ImGui::SameLine();
      Maj7ImGuiParamScaledFloat(
          filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::FreqKT), "KT##filt", 0, 1, 1, 1, 0, {});
      ImGui::SameLine();
      Maj7ImGuiParamFloat01(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::Q),
                            "Q##filt",
                            0,
                            0,
                            0,
                            lGetModInfo(M7::ModDestination::Filter1Q));

      ImGui::EndTabItem();
    }
  }

  const M7::OscillatorWaveformUiStyle& GetWaveformUiStyle(M7::OscillatorWaveform waveform) const
  {
    OSCILLATOR_WAVEFORM_UI_STYLES(gWaveformUiStyles);
    int idx = M7::math::ClampI((int)waveform, 0, (int)M7::OscillatorWaveform::Count - 1);
    return gWaveformUiStyles[idx];
  }

  static bool WaveformPreviewCacheMatches(const WaveformPreviewCacheEntry& entry,
                                          M7::OscillatorWaveform waveform,
                                          int width,
                                          int height,
                                          float waveshapeA01,
                                          float waveshapeB01,
                                          float phaseOffsetN11)
  {
    return entry.waveform == waveform && entry.width == width && entry.height == height &&
           entry.waveshapeA01 == waveshapeA01 && entry.waveshapeB01 == waveshapeB01 &&
           entry.phaseOffsetN11 == phaseOffsetN11;
  }

  static bool FilterPreviewCacheMatches(const FilterPreviewCacheEntry& entry,
                                        int filterIndex,
                                        int width,
                                        int height,
                                        M7::FilterResponse response,
                                        float cutoffHz,
                                        float reso01)
  {
    return entry.filterIndex == filterIndex && entry.width == width && entry.height == height &&
           entry.response == response &&
           entry.cutoffHz == cutoffHz && entry.reso01 == reso01;
  }

  FilterPreviewCacheEntry& GetOrBuildFilterPreviewCache(int filterIndex,
                                                        int width,
                                                        int height,
                                                        M7::FilterResponse response,
                                                        float cutoffHz,
                                                        float reso01)
  {
    for (auto& entry : mFilterPreviewCache)
    {
      if (FilterPreviewCacheMatches(entry, filterIndex, width, height, response, cutoffHz, reso01))
      {
        return entry;
      }
    }

    FilterPreviewCacheEntry newEntry;
    newEntry.filterIndex = filterIndex;
    newEntry.width = width;
    newEntry.height = height;
    newEntry.response = response;
    newEntry.cutoffHz = cutoffHz;
    newEntry.reso01 = reso01;
    newEntry.y01.resize((size_t)std::max(1, width));

    M7::FilterNode filterPreview;
    const float safeCutoff = M7::math::clamp(cutoffHz, 20.0f, 20000.0f);
    const float safeReso = M7::math::clamp01(reso01);
    filterPreview.SetParams(response, safeCutoff, M7::Param01{safeReso}, 0 /*gainDb*/);

    static constexpr float kMinFreq = 20.0f;
    static constexpr float kMaxFreq = 20000.0f;
    static constexpr float kFreqRatio = kMaxFreq / kMinFreq;
    static constexpr float kMinDb = -36.0f;
    static constexpr float kMaxDb = 12.0f;

    const int sampleCount = (int)newEntry.y01.size();
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    for (int i = 0; i < sampleCount; ++i)
    {
      const float t = (sampleCount <= 1) ? 0.0f : ((float)i / (float)(sampleCount - 1));
      const float freq = kMinFreq * M7::math::pow(kFreqRatio, t);
      const float mag = std::max(1e-6f, filterPreview.mFilter.GetMagnitudeAtFrequency(freq));
      const float db = M7::math::clamp(20.0f * M7::math::log10(mag), kMinDb, kMaxDb);
      const float yNorm = 1.0f - M7::math::clamp01(M7::math::lerp_rev(kMinDb, kMaxDb, db));
      newEntry.y01[(size_t)i] = yNorm;
    }
#else
    std::fill(newEntry.y01.begin(), newEntry.y01.end(), 0.55f);
#endif

    if (mFilterPreviewCache.size() < gFilterPreviewCacheMaxEntries)
    {
      mFilterPreviewCache.push_back(std::move(newEntry));
      return mFilterPreviewCache.back();
    }

    auto& evictTarget = mFilterPreviewCache[mFilterPreviewCacheEvictIndex];
    evictTarget = std::move(newEntry);
    mFilterPreviewCacheEvictIndex = (mFilterPreviewCacheEvictIndex + 1) % gFilterPreviewCacheMaxEntries;
    return evictTarget;
  }

  void DrawFilterPreview(const ImRect& bb, const FilterPreviewCacheEntry& cache)
  {
    if (cache.y01.empty())
      return;

    auto* drawList = ImGui::GetWindowDrawList();
    const float left = bb.Min.x + 4.0f;
    const float right = bb.Max.x - 4.0f;
    const float top = bb.Min.y + 4.0f;
    const float bottom = bb.Max.y - 4.0f;
    const float width = right - left;
    const float height = bottom - top;
    if (width <= 1.0f || height <= 1.0f)
      return;

    drawList->PushClipRect(bb.Min, bb.Max, true);
    ImU32 curveColor = ColorFromHTML("99bbd9", 0.45f);
    ImVec2 prev{};
    bool hasPrev = false;
    const int n = (int)cache.y01.size();
    for (int i = 0; i < n; ++i)
    {
      const float t = (n <= 1) ? 0.0f : ((float)i / (float)(n - 1));
      const float x = left + width * t;
      const float y = top + height * cache.y01[(size_t)i];
      ImVec2 cur{x, y};
      if (hasPrev)
      {
        drawList->AddLine(prev, cur, curveColor, 1.5f);
      }
      prev = cur;
      hasPrev = true;
    }
    drawList->PopClipRect();
  }

  WaveformPreviewCacheEntry& GetOrBuildWaveformPreviewCache(M7::OscillatorWaveform waveform,
                                                            int width,
                                                            int height,
                                                            float waveshapeA01,
                                                            float waveshapeB01,
                                                            float phaseOffsetN11)
  {
    for (auto& entry : mWaveformPreviewCache)
    {
      if (WaveformPreviewCacheMatches(entry, waveform, width, height, waveshapeA01, waveshapeB01, phaseOffsetN11))
      {
        return entry;
      }
    }

    WaveformPreviewCacheEntry newEntry;
    newEntry.waveform = waveform;
    newEntry.width = width;
    newEntry.height = height;
    newEntry.waveshapeA01 = waveshapeA01;
    newEntry.waveshapeB01 = waveshapeB01;
    newEntry.phaseOffsetN11 = phaseOffsetN11;
    newEntry.samples.resize((size_t)width);
    newEntry.minY = 50.0f;
    newEntry.maxY = -50.0f;

    std::unique_ptr<M7::OscillatorCore> waveformCore;
    waveformCore.reset(M7::InstantiateWaveformCore(waveform, WaveSabreCore::M7::OscillatorIntention::LFO));

    const float freqHz = Helpers::CurrentSampleRateF / (float)width;
    waveformCore->SetKRateParams(waveshapeA01, waveshapeB01, freqHz, false, 1);

    for (int sampleIndex = 0; sampleIndex < width; ++sampleIndex)
    {
      const auto result = waveformCore->renderSampleAndAdvance(phaseOffsetN11);
      const float sample = result.amplitude;
      newEntry.samples[(size_t)sampleIndex] = sample;
      if (sample < newEntry.minY)
        newEntry.minY = sample;
      if (sample > newEntry.maxY)
        newEntry.maxY = sample;
    }

    if (mWaveformPreviewCache.size() < gWaveformPreviewCacheMaxEntries)
    {
      mWaveformPreviewCache.push_back(std::move(newEntry));
      return mWaveformPreviewCache.back();
    }

    auto& evictTarget = mWaveformPreviewCache[mWaveformPreviewCacheEvictIndex];
    evictTarget = std::move(newEntry);
    mWaveformPreviewCacheEvictIndex = (mWaveformPreviewCacheEvictIndex + 1) % gWaveformPreviewCacheMaxEntries;
    return evictTarget;
  }

  void WaveformGraphic(M7::OscillatorWaveform waveform,
                       float waveshapeA01,
                       float waveshapeB01,
                       float phaseOffsetN11,
                       ImRect bb,
                       float* cursorPhase)
  {
    OSCILLATOR_WAVEFORM_UI_STYLES(gWaveformCaptions);

    float innerHeight = bb.GetHeight() - 4;

    ImVec2 outerTL = bb.Min;  // ImGui::GetCursorPos();
    ImVec2 outerBR = {outerTL.x + bb.GetWidth(), outerTL.y + bb.GetHeight()};

    const int sampleCount = std::max(1, (int)bb.GetWidth());
    waveform = (M7::OscillatorWaveform)M7::math::ClampI((int)waveform, 0, (int)M7::OscillatorWaveform::Count - 1);
    auto& cachedPreview =
      GetOrBuildWaveformPreviewCache(waveform, sampleCount, (int)bb.GetHeight(), waveshapeA01, waveshapeB01, phaseOffsetN11);

    auto drawList = ImGui::GetWindowDrawList();

    auto sampleToY = [&](float sample)
    {
      float c = outerBR.y - float(bb.GetHeight()) * 0.5f;
      float h = float(innerHeight) * 0.5f * sample;
      return c - h;
    };
    auto phaseToX = [&](float phase)
    {
      return outerTL.x + M7::math::fract(phase) * bb.GetWidth();
    };

    auto waveformUiStyle = GetWaveformUiStyle(waveform);
    ImGui::RenderFrame(outerTL, outerBR, ColorFromHTML(waveformUiStyle.backgroundColorHtml), true, 3.0f);  // background
    drawList->PushClipRect(bb.Min, bb.Max, true);
    ImU32 waveformColor = ColorFromHTML(waveformUiStyle.foregroundColorHtml);

    float centerY = sampleToY(0);
    drawList->AddLine({outerTL.x, centerY},
                      {outerBR.x, centerY},
                      ImGui::GetColorU32(ImGuiCol_PlotLines),
                      2.0f);  // center line
    for (int iSample = 0; iSample < sampleCount; ++iSample)
    {
      const float sample = cachedPreview.samples[(size_t)iSample];

      drawList->AddLine({outerTL.x + iSample, centerY}, {outerTL.x + iSample, sampleToY(sample)}, waveformColor, 1);
    }

    if (cursorPhase != nullptr)
    {
      float x = phaseToX(*cursorPhase - phaseOffsetN11);
      static constexpr float gHalfCursorWidth = 2.5f * 0.5f;
      drawList->AddLine({x - gHalfCursorWidth, outerTL.y},
                        {x + gHalfCursorWidth, outerBR.y},
                        ColorFromHTML("#ff0000"),
                        2.0f);  // center line
    }
    drawList->PopClipRect();

    {
      //auto str1 = std::format("nrg:[{:.2f},{:.2f}]", cachedPreview.minY, cachedPreview.maxY);
      //auto str2 = std::format("dc :{:.2f}", pWaveform->mDCOffset);
      //auto str3 = std::format("amp:{:.2f}", pWaveform->mScale);
      //auto str4 = std::format("org:[{:.2f},{:.2f}]", ominY, omaxY);

      //DrawShadowText(str1, {bb.Min.x, bb.Min.y + 12});
      //DrawShadowText(str4, {bb.Min.x, bb.Min.y + 24});
      //DrawShadowText(str2, {bb.Min.x, bb.Min.y + 24});
      //DrawShadowText(str3, {bb.Min.x, bb.Min.y + 36});
    }

    DrawShadowText(gWaveformCaptions[(int)waveform].label, bb.Min);
  }

  void DrawShadowText(const std::string& text, const ImVec2& pos)
  {
    auto drawList = ImGui::GetWindowDrawList();
    auto bgcol = ColorFromHTML("ffffff", 0.5f);
    auto fgcol = ColorFromHTML("000000", 1);
    //drawList->AddText({ pos.x + 1, pos.y + 1 }, bgcol, text.c_str());
    //drawList->AddText({ pos.x - 1, pos.y - 1 }, bgcol, text.c_str());
    //drawList->AddText({ pos.x - 1, pos.y + 1 }, bgcol, text.c_str());
    //drawList->AddText({ pos.x + 1, pos.y - 1 }, bgcol, text.c_str());

    auto textSize = ImGui::CalcTextSize(text.c_str());

    // draw a box behind the text for better visibility
    //drawList->AddRectFilled({pos.x - 2, pos.y - 2}, {pos.x + textSize.x + 2, pos.y + textSize.y + 2}, bgcol, 3.0f);
    drawList->AddRectFilled(pos, pos + textSize, bgcol, 3.0f);

    drawList->AddText(pos, fgcol, text.c_str());
  }

  bool WaveformButton(ImGuiID id,
                      M7::OscillatorWaveform waveform,
                      float waveshapeA01,
                      float waveshapeB01,
                      float phaseOffsetN11,
                      float* phaseCursor,
                      bool isSelected = false)
  {
    id += 10;  // &= 0x8000; // just to comply with ImGui expectations of IDs never being 0.
    ImGuiButtonFlags flags = 0;
    ImGuiContext& g = *GImGui;
    const ImVec2 size = {150, 80};
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
      return false;

    const ImVec2 padding = g.Style.FramePadding;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2.0f);
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
      return false;

    bool hovered, held;

    WaveformGraphic(waveform, waveshapeA01, waveshapeB01, phaseOffsetN11, bb, phaseCursor);

    if (isSelected)
    {
      auto waveformUiStyle = GetWaveformUiStyle(waveform);
      ImGui::GetWindowDrawList()->AddRect(
          bb.Min, bb.Max, ColorFromHTML(waveformUiStyle.foregroundColorHtml), 3.0f, 0, 2.0f);
    }

    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

    return pressed;
  }

  void WaveformParam(int waveformParamID,
                     int waveshapeAParamID,
                     int waveshapeBParamId,
                     int phaseOffsetParamID,
                     float* phaseCursor)
  {
    //OSCILLATOR_WAVEFORM_UI_STYLES(gWaveformCaptions);

    M7::QuickParam waveformParam{pMaj7->mParamCache[waveformParamID]};

    //M7::EnumParam<M7::OscillatorWaveform> waveformParam(pMaj7->mParamCache[waveformParamID], M7::OscillatorWaveform::Count);
    //M7::Float01Param waveshapeParam(pMaj7->mParamCache[waveshapeParamID]);
    M7::OscillatorWaveform selectedWaveform = waveformParam.GetEnumValue<M7::OscillatorWaveform>();
    float waveshapeA01 = GetEffectX()->getParameter((VstInt32)waveshapeAParamID);  // waveshapeParam.Get01Value();
    float waveshapeB01 = GetEffectX()->getParameter((VstInt32)waveshapeBParamId);  // waveshapeParam.Get01Value();
    //M7::FloatN11Param phaseOffsetParam(pMaj7->mParamCache[phaseOffsetParamID]);
    M7::QuickParam phaseOffsetParam{GetEffectX()->getParameter((VstInt32)phaseOffsetParamID)};
    float phaseOffsetN11 = phaseOffsetParam.GetN11Value();

    auto setWaveformByIndex = [&](int index)
    {
      const int waveformCount = (int)M7::OscillatorWaveform::Count;
      if (waveformCount <= 0)
        return;
      while (index < 0)
        index += waveformCount;
      while (index >= waveformCount)
        index -= waveformCount;
      M7::QuickParam t{};
      GetEffectX()->setParameter(waveformParamID, t.SetEnumValue((M7::OscillatorWaveform)index));
      selectedWaveform = (M7::OscillatorWaveform)index;
    };

    {
      ImGuiGroupScope groupScopeWF;

      if (WaveformButton(
              waveformParamID, selectedWaveform, waveshapeA01, waveshapeB01, phaseOffsetN11, phaseCursor, true))
      {
        ImGui::OpenPopup("selectWaveformPopup");
      }
      ImGui::SameLine();

      {
        ImGuiGroupScope groupScopeSmallButtons;
        ImGui::PushID(waveformParamID);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 0));
        if (ImGui::SmallButton("+"))
        {
          setWaveformByIndex((int)selectedWaveform + 1);
        }
        if (ImGui::SmallButton("-"))
        {
          setWaveformByIndex((int)selectedWaveform - 1);
        }
        ImGui::PopStyleVar();
        ImGui::PopID();
      }
    }

    if (ImGui::BeginPopup("selectWaveformPopup"))
    {
      for (int n = 0; n < (int)M7::OscillatorWaveform::Count; n++)
      {
        M7::OscillatorWaveform wf = (M7::OscillatorWaveform)n;
        auto wfUiStyle = GetWaveformUiStyle(wf);
        if (n > 0 && !wfUiStyle.startNewRow)
        {
          ImGui::SameLine();
        }

        ImGui::PushID(n);
        if (WaveformButton(n, wf, waveshapeA01, waveshapeB01, phaseOffsetN11, phaseCursor, wf == selectedWaveform))
        {
          //float t;
          //M7::EnumParam<M7::OscillatorWaveform> tp(t, M7::OscillatorWaveform::Count);
          //tp.SetEnumValue(wf);
          M7::QuickParam t{};
          GetEffectX()->setParameter(waveformParamID, t.SetEnumValue(wf));
        }
        ImGui::PopID();
      }
      ImGui::EndPopup();
    }
  }  // waveform param


#ifdef MAJ7_INCLUDE_GSM_SUPPORT
  bool LoadSample(const char* path, M7::SamplerDevice& sampler, size_t isrc)
  {
    std::ifstream input(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!input.is_open())
      return SetStatus(isrc, StatusStyle::Error, "Could not open file.");
    size_t inputSize = (size_t)input.tellg();
    auto inputBuf = std::make_unique<char[]>(inputSize);  // new unsigned char[(unsigned int)inputSize];
    input.seekg(0, std::ios::beg);
    input.read((char*)inputBuf.get(), inputSize);
    input.close();

    if (*((unsigned int*)inputBuf.get()) != 0x46464952)
      return SetStatus(isrc, StatusStyle::Error, "Input file missing RIFF header.");
    if (*((unsigned int*)(inputBuf.get() + 4)) != (unsigned int)inputSize - 8)
      return SetStatus(isrc, StatusStyle::Error, "Input file contains invalid RIFF header.");
    if (*((unsigned int*)(inputBuf.get() + 8)) != 0x45564157)
      return SetStatus(isrc, StatusStyle::Error, "Input file missing WAVE chunk.");

    if (*((unsigned int*)(inputBuf.get() + 12)) != 0x20746d66)
      return SetStatus(isrc, StatusStyle::Error, "Input file missing format sub-chunk.");
    if (*((unsigned int*)(inputBuf.get() + 16)) != 16)
      return SetStatus(isrc, StatusStyle::Error, "Input file is not a PCM waveform.");
    auto inputFormat = (LPWAVEFORMATEX)(inputBuf.get() + 20);
    if (inputFormat->wFormatTag != WAVE_FORMAT_PCM)
      return SetStatus(isrc, StatusStyle::Error, "Input file is not a PCM waveform.");
    if (inputFormat->nChannels != 1)
      return SetStatus(isrc, StatusStyle::Error, "Input file is not mono.");
    if (inputFormat->wBitsPerSample != sizeof(short) * 8)
      return SetStatus(isrc, StatusStyle::Error, "Input file is not 16-bit.");

    int chunkPos = 36;
    int chunkSizeBytes;
    while (true)
    {
      if (chunkPos >= (int)inputSize)
        return SetStatus(isrc, StatusStyle::Error, "Input file missing data sub-chunk.");
      chunkSizeBytes = *((unsigned int*)(inputBuf.get() + chunkPos + 4));
      if (*((unsigned int*)(inputBuf.get() + chunkPos)) == 0x61746164)
        break;
      else
        chunkPos += 8 + chunkSizeBytes;
    }
    int rawDataLength = chunkSizeBytes / 2;

    auto rawData = std::make_unique<short[]>(rawDataLength);

    memcpy(rawData.get(), inputBuf.get() + chunkPos + 8, chunkSizeBytes);

    auto compressedData = std::make_unique<char[]>(chunkSizeBytes);  // new char[chunkSizeBytes];

    int waveFormatSize = 0;
    acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &waveFormatSize);
    auto waveFormatBuf = std::make_unique<char[]>(waveFormatSize);
    auto waveFormat = (WAVEFORMATEX*)waveFormatBuf.get();  // (new char[waveFormatSize]);
    memset(waveFormat, 0, waveFormatSize);
    waveFormat->wFormatTag = WAVE_FORMAT_GSM610;
    waveFormat->nSamplesPerSec = inputFormat->nSamplesPerSec;  // Specimen::SampleRate;

    ACMFORMATCHOOSE formatChoose;
    memset(&formatChoose, 0, sizeof(formatChoose));
    formatChoose.cbStruct = sizeof(formatChoose);
    formatChoose.pwfx = waveFormat;
    formatChoose.cbwfx = waveFormatSize;
    formatChoose.pwfxEnum = waveFormat;
    formatChoose.fdwEnum = ACM_FORMATENUMF_WFORMATTAG | ACM_FORMATENUMF_NSAMPLESPERSEC;

    if (acmFormatChoose(&formatChoose))
      return SetStatus(isrc, StatusStyle::Error, "acmFormatChoose failed");

    acmDriverEnum(driverEnumCallback, (DWORD_PTR)waveFormat, NULL);
    HACMDRIVER driver = NULL;
    if (acmDriverOpen(&driver, driverId, 0))
      return SetStatus(isrc, StatusStyle::Error, "acmDriverOpen failed");

    HACMSTREAM stream = NULL;
    if (acmStreamOpen(&stream, driver, inputFormat, waveFormat, NULL, NULL, NULL, ACM_STREAMOPENF_NONREALTIME))
      return SetStatus(isrc, StatusStyle::Error, "acmStreamOpen failed");

    ACMSTREAMHEADER streamHeader;
    memset(&streamHeader, 0, sizeof(streamHeader));
    streamHeader.cbStruct = sizeof(streamHeader);
    streamHeader.pbSrc = (LPBYTE)rawData.get();
    streamHeader.cbSrcLength = chunkSizeBytes;
    streamHeader.pbDst = (LPBYTE)compressedData.get();
    streamHeader.cbDstLength = chunkSizeBytes;
    if (acmStreamPrepareHeader(stream, &streamHeader, 0))
      return SetStatus(isrc, StatusStyle::Error, "acmStreamPrepareHeader failed");
    if (acmStreamConvert(stream, &streamHeader, 0))
      return SetStatus(isrc, StatusStyle::Error, "acmStreamConvert failed");

    acmStreamClose(stream, 0);
    acmDriverClose(driver, 0);

    sampler.LoadSample(
        compressedData.get(), streamHeader.cbDstLengthUsed, chunkSizeBytes, waveFormat, ::PathFindFileNameA(path));

    return SetStatus(isrc, StatusStyle::Green, "Sample loaded successfully.");
  }
#endif  // MAJ7_INCLUDE_GSM_SUPPORT

  static BOOL __stdcall driverEnumCallback(HACMDRIVERID driverId, DWORD_PTR dwInstance, DWORD fdwSupport)
  {
    ACMDRIVERDETAILS driverDetails;
    driverDetails.cbStruct = sizeof(driverDetails);
    acmDriverDetails(driverId, &driverDetails, 0);

    HACMDRIVER driver = NULL;
    acmDriverOpen(&driver, driverId, 0);

    int waveFormatSize = 0;
    acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &waveFormatSize);
    auto waveFormatBuf = std::make_unique<uint8_t[]>(waveFormatSize);
    auto pWaveFormat = (WAVEFORMATEX*)waveFormatBuf.get();
    ACMFORMATDETAILS formatDetails;
    memset(&formatDetails, 0, sizeof(formatDetails));
    formatDetails.cbStruct = sizeof(formatDetails);
    formatDetails.pwfx = pWaveFormat;
    formatDetails.cbwfx = waveFormatSize;
    formatDetails.dwFormatTag = WAVE_FORMAT_UNKNOWN;
    acmFormatEnum(driver, &formatDetails, formatEnumCallback, dwInstance, NULL);
    return 1;
  }

  static BOOL __stdcall formatEnumCallback(HACMDRIVERID __driverId,
                                           LPACMFORMATDETAILS formatDetails,
                                           DWORD_PTR dwInstance,
                                           DWORD fdwSupport)
  {
    auto waveFormat = (WAVEFORMATEX*)dwInstance;

    if (!memcmp(waveFormat, formatDetails->pwfx, sizeof(WAVEFORMATEX)))
    {
      driverId = __driverId;
      foundWaveFormat = formatDetails->pwfx;
    }

    return 1;
  }

  static HACMDRIVERID driverId;
  static WAVEFORMATEX* foundWaveFormat;


  //std::vector<std::pair<std::string, int>> autocomplete(std::string input, const std::vector<std::pair<std::string, int>>& entries) {
  //	std::vector<std::pair<std::string, int>> suggestions;
  //	std::transform(input.begin(), input.end(), input.begin(), ::tolower); // convert input to lowercase
  //	for (auto entry : entries) {
  //		std::string lowercaseEntry = entry.first;
  //		std::transform(lowercaseEntry.begin(), lowercaseEntry.end(), lowercaseEntry.begin(), ::tolower); // convert entry to lowercase
  //		int inputIndex = 0, entryIndex = 0;
  //		while ((size_t)inputIndex < input.size() && (size_t)entryIndex < lowercaseEntry.size()) {
  //			if (input[inputIndex] == lowercaseEntry[entryIndex]) {
  //				inputIndex++;
  //			}
  //			entryIndex++;
  //		}
  //		if (inputIndex == input.size()) {
  //			suggestions.push_back(entry);
  //		}
  //	}
  //	return suggestions;
  //}

  void Sampler(const char* labelWithID, M7::SamplerDevice& sampler, size_t isrc, int ampEnvModDestBase)
  {
    ColorMod& cm = sampler.IsEnabled() ? mSamplerColors : mSamplerDisabledColors;
    auto token = cm.Push();
    static constexpr char const* const interpModeNames[] = {"Nearest", "Linear"};
    static constexpr char const* const loopModeNames[] = {"Disabled", "Repeat", "Pingpong"};
    static constexpr char const* const loopBoundaryModeNames[] = {"FromSample", "Manual"};

    auto lGetModInfo = [&](M7::SamplerModParamIndexOffsets x)
    {
      return GetModInfo((M7::ModDestination)((int)pMaj7->mSources[isrc]->mModDestBaseID + (int)x));
    };


    if (WSBeginTabItemWithSel(labelWithID, (int)isrc, mSourceTabSelHelper))
    {
      //WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Enabled), "Enabled");
      Maj7ImGuiBoolParamToggleButton2((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Enabled),
                                     "On ", "Off");
      ImGui::SameLine();
      Maj7ImGuiParamVolume((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Volume),
                           "Volume",
                           M7::gUnityVolumeCfg,
                           0,
                           lGetModInfo(M7::SamplerModParamIndexOffsets::Volume));

                           ImGui::SameLine();
      Maj7ImGuiParamVolume((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::CompensationGain),
                           "Comp.Vol",
                           M7::gVolumeCfg12db,
                           0,
                           {});

      ImGui::SameLine(0, 50);
      Maj7ImGuiParamFrequency((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqParam),
                              (int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqKT),
                              "Freq",
                              M7::gSourceFreqConfig,
                              M7::gFreqParamKTUnity,
                              lGetModInfo(M7::SamplerModParamIndexOffsets::FrequencyParam));
      ImGui::SameLine();
      Maj7ImGuiParamScaledFloat(
          (int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqKT), "KT", 0, 1, 1, 1, 0, {});
      ImGui::SameLine();
      Maj7ImGuiParamInt((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::TuneSemis),
                        "Transp",
                        M7::gSourcePitchSemisRange,
                        0,
                        0);
      ImGui::SameLine();
      Maj7ImGuiParamFloatN11((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::TuneFine),
                             "FineTune",
                             0,
                             0,
                             lGetModInfo(M7::SamplerModParamIndexOffsets::PitchFine));
      ImGui::SameLine();
      Maj7ImGuiParamInt((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::BaseNote),
                        "BaseNote",
                        M7::gKeyRangeCfg,
                        60,
                        60);

      //ImGui::SameLine(0, 50); WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LegatoTrig), "Leg.Trig");
      ImGui::SameLine(0, 50);
      Maj7ImGuiBoolParamToggleButton((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LegatoTrig),
                                     "Leg.Trig");

      ImGui::SameLine();
      ImGui::BeginGroup();
      Maj7ImGuiParamMidiNote((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::KeyRangeMin),
                             "KeyMin",
                             0,
                             0);
      Maj7ImGuiParamMidiNote((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::KeyRangeMax),
                             "KeyMax",
                             127,
                             127);
      ImGui::EndGroup();

      ImGui::SameLine(0, 50);
      Maj7ImGuiParamEnumList<WaveSabreCore::LoopMode>((int)sampler.mParams.GetParamIndex(
                                                          M7::SamplerParamIndexOffsets::LoopMode),
                                                      "LoopMode##mst",
                                                      (int)WaveSabreCore::LoopMode::NumLoopModes,
                                                      WaveSabreCore::LoopMode::Repeat,
                                                      loopModeNames);
      ImGui::SameLine();
      Maj7ImGuiParamEnumList<WaveSabreCore::LoopBoundaryMode>(
          (int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopSource),
          "LoopSrc##mst",
          (int)WaveSabreCore::LoopBoundaryMode::NumLoopBoundaryModes,
          WaveSabreCore::LoopBoundaryMode::FromSample,
          loopBoundaryModeNames);
      //ImGui::SameLine(); WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::ReleaseExitsLoop), "Rel");
      ImGui::SameLine();
      Maj7ImGuiBoolParamToggleButton((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::ReleaseExitsLoop),
                                     "Rel");
      // ImGui::SameLine();
      // Maj7ImGuiParamEnumList<WaveSabreCore::InterpolationMode>(
      //     (int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::InterpolationType),
      //     "Interp.##mst",
      //     (int)WaveSabreCore::InterpolationMode::NumInterpolationModes,
      //     WaveSabreCore::InterpolationMode::Linear,
      //     interpModeNames);

      ImGui::SameLine();
      Maj7ImGuiParamFloatN11((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Pan),
                             "Pan",
                             0,
                             0,
                             lGetModInfo(M7::SamplerModParamIndexOffsets::Pan));

      ImGui::BeginGroup();
      //WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Reverse), "Reverse");
      Maj7ImGuiBoolParamToggleButton((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Reverse),
                                     "Reverse");

      ImGui::SameLine();
      Maj7ImGuiParamFloat01(sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::SampleStart),
                            "SampleStart",
                            0,
                            0,
                            0,
                            lGetModInfo(M7::SamplerModParamIndexOffsets::SampleStart));
      ImGui::SameLine();
      Maj7ImGuiPowCurvedParam(sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Delay),
                              "SampleDelay",
                              M7::gEnvTimeCfg,
                              0,
                              lGetModInfo(M7::SamplerModParamIndexOffsets::Delay));

      ImGui::SameLine();
      WSImGuiParamKnob((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopStart), "LoopBeg");
      ImGui::SameLine();
      WSImGuiParamKnob((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopLength), "LoopLen");
      ImGui::EndGroup();

      ImGui::SameLine();
      SamplerWaveformDisplay(sampler, isrc);

#ifdef MAJ7_INCLUDE_GSM_SUPPORT

      if (ImGui::SmallButton("Load from file ..."))
      {
        OPENFILENAMEA ofn = {0};
        char szFile[MAX_PATH] = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = this->mCurrentWindow;
        ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
        if (GetOpenFileNameA(&ofn))
        {
          mSourceStatusText[isrc].mStatus.clear();
          LoadSample(szFile, sampler, isrc);
        }
      }
#endif  // MAJ7_INCLUDE_GSM_SUPPORT 
      if (!mShowingGmDlsList)
      {
        //ImGui::SameLine();
        if (ImGui::SmallButton("Load Gm.Dls sample..."))
        {
          mShowingGmDlsList = true;
        }
      }
      else
      {
        ImGui::InputText("Filter#gmdlslist", mGmDlsFilter, std::size(mGmDlsFilter));
        ImGui::BeginListBox("sample");
        int sampleIndex = (int)sampler.mParams.GetIntValue(M7::SamplerParamIndexOffsets::GmDlsIndex);

        auto matches = (std::strlen(mGmDlsFilter) > 0) ? autocomplete(mGmDlsFilter, mGmDlsOptions) : mGmDlsOptions;
        for (auto& x : matches)
        {
          bool isSelected = sampleIndex == x.second;
          if (ImGui::Selectable(x.first.c_str(), &isSelected))
          {
            if (isSelected)
            {
              sampler.LoadGmDlsSample(x.second);
            }
          }
        }
        ImGui::EndListBox();
        if (ImGui::SmallButton("Close"))
        {
          mShowingGmDlsList = false;
        }
      }

      //ImGui::SameLine();
      //auto sampleSource = sampler.mParams.GetEnumValue<M7::SampleSource>(M7::SamplerParamIndexOffsets::SampleSource);
      //if (!sampler.mSample)
      //{
      //  ImGui::Text("No sample loaded");
      //}
//      else if (sampleSource == M7::SampleSource::Embed)
//      {
//#ifdef MAJ7_INCLUDE_GSM_SUPPORT
//          auto* p = static_cast<WaveSabreCore::M7::GsmSample*>(sampler.mSample);
//        ImGui::Text("Uncompressed size: %d, compressed to %d (%d%%) / %d Samples / path:%s",
//                    p->UncompressedSize,
//                    p->CompressedSize,
//                    (p->CompressedSize * 100) / p->UncompressedSize,
//                    p->SampleLength,
//                    sampler.mSamplePath);
//
//        if (ImGui::SmallButton("Clear sample"))
//        {
//          sampler.Reset();
//        }
//#endif  // MAJ7_INCLUDE_GSM_SUPPORT
//      }
//      else if (sampleSource == M7::SampleSource::GmDls)
//      {
        //auto* p = static_cast<M7::GmDlsSample*>(sampler.mSample);
      auto* p = &sampler.mSample;
        const char* name = "(none)";
        if (p->mSampleIndex >= 0 && p->mSampleIndex < M7::gGmDlsSampleCount)
        {
          name = mGmDlsOptions[p->mSampleIndex + 1].first.c_str();
        }
        ImGui::Text("GmDls : %s (%d)", name, p->mSampleIndex);
      //}
      //else
      //{
      //  ImGui::Text("--");
      //}

      ImGui::SameLine();
      switch (mSourceStatusText[isrc].mStatusStyle)
      {
        case StatusStyle::NoStyle:
          ImGui::Text("%s", mSourceStatusText[isrc].mStatus.c_str());
          break;
        case StatusStyle::Error:
          ImGui::TextColored(ImColor::HSV(0 / 7.0f, 0.6f, 0.6f), "%s", mSourceStatusText[isrc].mStatus.c_str());
          break;
        case StatusStyle::Warning:
          ImGui::TextColored(ImColor::HSV(1 / 7.0f, 0.6f, 0.6f), "%s", mSourceStatusText[isrc].mStatus.c_str());
          break;
        default:
        case StatusStyle::Green:
          ImGui::TextColored(ImColor::HSV(2 / 7.0f, 0.6f, 0.6f), "%s", mSourceStatusText[isrc].mStatus.c_str());
          break;
      }

      {
        //ColorMod::Token ampEnvColorModToken{ (ampSourceParam.GetIntValue() != oscID) ? mPinkColors.mNewColors : nullptr };
        Envelope("Amplitude Envelope",
                 (int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::AmpEnvDelayTime),
                 ampEnvModDestBase);
      }
      ImGui::EndTabItem();
    }  // if begin tab item
  }  // sampler()


  void WaveformGraphic(size_t isrc,
                       float height,
                       const std::vector<std::pair<float, float>>& peaks,
                       float startPos01,
                       float loopStart01,
                       float loopLen01,
                       float cursor)
  {
    ImGuiContext& g = *GImGui;
    size_t nSamples = peaks.size();
    const ImVec2 size = {(float)nSamples, height};
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 padding = g.Style.FramePadding;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2.0f);

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID((int)isrc)))
    {
      return;
    }

    float innerHeight = bb.GetHeight() - 4;

    ImVec2 outerTL = bb.Min;  // ImGui::GetCursorPos();
    ImVec2 outerBR = {outerTL.x + bb.GetWidth(), outerTL.y + bb.GetHeight()};

    auto drawList = ImGui::GetWindowDrawList();

    auto sampleToY = [&](float sample)
    {
      float c = outerBR.y - float(bb.GetHeight()) * 0.5f;
      float h = float(innerHeight) * 0.5f * sample;
      return c - h;
    };

    ImGui::RenderFrame(outerTL, outerBR, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f);  // background
    float centerY = sampleToY(0);
    drawList->AddLine({outerTL.x, centerY},
                      {outerBR.x, centerY},
                      ImGui::GetColorU32(ImGuiCol_PlotLines),
                      2.0f);  // center line
    for (size_t iSample = 0; iSample < nSamples; ++iSample)
    {
      auto minY = sampleToY(peaks[iSample].first) - 0.5f;
      auto maxY = sampleToY(peaks[iSample].second) + 0.5f;
      drawList->AddLine({outerTL.x + iSample, minY},
                        {outerTL.x + iSample, maxY},
                        ImGui::GetColorU32(ImGuiCol_PlotHistogram),
                        1);
    }

    auto GetX = [&](float t01)
    {
      float x = M7::math::lerp(bb.Min.x, bb.Max.x, t01);
      x = M7::math::clamp(x, bb.Min.x, bb.Max.x);
      return x;
    };

    auto drawCursor = [&](float t01, ImU32 color)
    {
      auto x = GetX(t01);
      drawList->AddLine({x, bb.Min.y}, {x, bb.Max.y}, color, 3.5f);
    };

    drawCursor(loopStart01, ColorFromHTML("a03030"));
    drawCursor(loopStart01 + loopLen01, ColorFromHTML("a03030"));
    drawList->AddRectFilled({GetX(loopStart01), bb.Min.y},
                            {GetX(loopStart01 + loopLen01), bb.Max.y},
                            ColorFromHTML("a03030", 0.4f));
    drawList->AddRectFilled({GetX(loopStart01), bb.Max.y - 10},
                            {GetX(loopStart01 + loopLen01), bb.Max.y},
                            ColorFromHTML("a03030", 0.8f));

    drawCursor(startPos01, ColorFromHTML("30a030"));
    drawCursor(cursor, ColorFromHTML("3030aa"));
  }

  // TODO: cache this image in a texture.
  void SamplerWaveformDisplay(M7::SamplerDevice& sampler, size_t isrc)
  {
    auto sourceInfo = this->mSourceStatusText[isrc];
    //ImGuiIO& io = ImGui::GetIO();
    //ImGui::Image(io.Fonts->TexID, { gSamplerWaveformWidth, gSamplerWaveformHeight });
    //if (!sampler.mSample)
    //  return;

    auto sampleDataPair = DupeBuffer<float>(sampler.mSample.mSampleData.mBuffer);
    auto sampleData = sampleDataPair.first;
    auto sampleLengthSamples = sampleDataPair.second;

    std::vector<std::pair<float, float>> peaks;
    peaks.resize(gSamplerWaveformWidth);
    for (size_t i = 0; i < gSamplerWaveformWidth; ++i)
    {
      size_t sampleBegin = size_t((i * sampleLengthSamples) / gSamplerWaveformWidth);
      size_t sampleEnd = size_t(((i + 1) * sampleLengthSamples) / gSamplerWaveformWidth);
      sampleEnd = std::max(sampleEnd, sampleBegin + 1);
      peaks[i].first = 0;
      peaks[i].second = 0;
      for (size_t s = sampleBegin; s < sampleEnd; ++s)
      {
        peaks[i].first = std::min(peaks[i].first, sampleData[s]);
        peaks[i].second = std::max(peaks[i].second, sampleData[s]);
      }
    }

    double cursor = 0;
    if (mRenderContext.mpActiveVoice)
    {
      M7::SamplerVoice* sv = static_cast<M7::SamplerVoice*>(mRenderContext.mpActiveVoice->mSourceVoices[isrc]);
      cursor = sv->mSamplePlayer.samplePos;
      cursor /= sampleLengthSamples;
    }

    auto sampleStart = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::SampleStart);
    auto loopStart = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::LoopStart);
    auto loopLength = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::LoopLength);
    WaveformGraphic(isrc, gSamplerWaveformHeight, peaks, sampleStart, loopStart, loopLength, (float)cursor);

    FreeBuffer(sampleDataPair.first);
  }

};  // class maj7editor
