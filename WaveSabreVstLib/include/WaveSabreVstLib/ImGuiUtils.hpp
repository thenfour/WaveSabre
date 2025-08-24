
#pragma once

namespace WaveSabreVstLib
{
struct ImGuiGroupScope
{
  bool mIdSet = false;
  explicit ImGuiGroupScope(int pushId)
      : mIdSet(true)
  {
    ImGui::BeginGroup();
    ImGui::PushID(pushId);
  }
  explicit ImGuiGroupScope(const char* pushId)
      : mIdSet(true)
  {
    ImGui::BeginGroup();
    ImGui::PushID(pushId);
  }
  ImGuiGroupScope()
  {
    ImGui::BeginGroup();
  }
  ~ImGuiGroupScope()
  {
    if (mIdSet)
    {
      ImGui::PopID();
    }
    ImGui::EndGroup();
  }
};

struct ImGuiEnabledScope
{
  bool mWasDisabled = false;
  ImGuiEnabledScope(bool enabled)
  {
    if (!enabled)
    {
      mWasDisabled = true;
      ImGui::BeginDisabled();
    }
  }
  ~ImGuiEnabledScope()
  {
    if (mWasDisabled)
    {
      ImGui::EndDisabled();
    }
  }
};

struct ImGuiIdScope
{
  explicit ImGuiIdScope(int pushId)
  {
    ImGui::PushID(pushId);
  }
  explicit ImGuiIdScope(size_t pushId)
  {
    ImGui::PushID((int)pushId);
  }
  explicit ImGuiIdScope(const char* pushId)
  {
    ImGui::PushID(pushId);
  }
  ~ImGuiIdScope()
  {
    ImGui::PopID();
  }
};

// RAII for pushing ImGuiStyleColors.
struct ImGuiStyleColorScope
{
  explicit ImGuiStyleColorScope(ImGuiCol idx, const ImVec4& col)
  {
    ImGui::PushStyleColor(idx, col);
  }
  ~ImGuiStyleColorScope()
  {
    ImGui::PopStyleColor();
  }
};

// RAII for pushing ImGuiStyleVar
struct ImGuiStyleVarScope
{
  explicit ImGuiStyleVarScope(ImGuiStyleVar idx, float val)
  {
    ImGui::PushStyleVar(idx, val);
  }
  explicit ImGuiStyleVarScope(ImGuiStyleVar idx, const ImVec2& val)
  {
    ImGui::PushStyleVar(idx, val);
  }
  ~ImGuiStyleVarScope()
  {
    ImGui::PopStyleVar();
  }
};

struct ImGuiScope
{
  int mStyleColorCount = 0;
  int mStyleVarCount = 0;
  int mIdCount = 0;

  void PushStyleColor(ImGuiCol idx, const ImVec4& col)
  {
    ImGui::PushStyleColor(idx, col);
    ++mStyleColorCount;
  }
  void PushStyleVar(ImGuiStyleVar idx, float val)
  {
    ImGui::PushStyleVar(idx, val);
    ++mStyleVarCount;
  }
  void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
  {
    ImGui::PushStyleVar(idx, val);
    ++mStyleVarCount;
  }
  void PushID(int id)
  {
    ImGui::PushID(id);
    ++mIdCount;
  }
  void PushID(const char* id)
  {
    ImGui::PushID(id);
    ++mIdCount;
  }
  ~ImGuiScope()
  {
    for (int i = 0; i < mIdCount; ++i)
      ImGui::PopID();
    for (int i = 0; i < mStyleVarCount; ++i)
      ImGui::PopStyleVar();
    for (int i = 0; i < mStyleColorCount; ++i)
      ImGui::PopStyleColor();
  }
};



}  // namespace WaveSabreVstLib
