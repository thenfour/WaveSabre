#pragma  once

#include <Windows.h>

namespace WaveSabreCore
{

class CriticalSection
{
public:
  class CriticalSectionGuard
  {
  public:
    CriticalSectionGuard(CriticalSection* criticalSection)
        : criticalSection(criticalSection)
    {
      EnterCriticalSection(&criticalSection->criticalSection);
    }
    ~CriticalSectionGuard()
    {
      LeaveCriticalSection(&criticalSection->criticalSection);
    }

  private:
    CriticalSection* criticalSection;
  };

  CriticalSection()
  {
    InitializeCriticalSection(&criticalSection);
  }
  ~CriticalSection()
  {
    DeleteCriticalSection(&criticalSection);
  }

  CriticalSectionGuard Enter()
  {
    return CriticalSectionGuard(this);
  }

  void ManualEnter()
  {
    EnterCriticalSection(&criticalSection);
  }
  void ManualLeave()
  {
    LeaveCriticalSection(&criticalSection);
  }

private:
  CRITICAL_SECTION criticalSection;
};

} // namespace WaveSabreCore