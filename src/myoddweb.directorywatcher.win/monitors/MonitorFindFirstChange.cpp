#include "MonitorFindFirstChange.h"
#include <string>
#include <Windows.h>

MonitorFindFirstChange::MonitorFindFirstChange(__int64 id) :
  Monitor(id)
{
}

MonitorFindFirstChange::~MonitorFindFirstChange()
{
}

void MonitorFindFirstChange::Poll(std::wstring path, bool recursive) const
{
  auto changeHandle = FindFirstChangeNotificationW(path.c_str(), recursive ? TRUE : FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
  for (;;)
  {
    auto wait = WaitForSingleObject(changeHandle, INFINITE);
    if (wait == WAIT_OBJECT_0)
    {
      FindNextChangeNotification(changeHandle);
    }
    else
    {
      break;
    }
  }
  FindCloseChangeNotification(changeHandle);
}