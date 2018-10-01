#pragma once
#include <mutex>
#include <unordered_map>
#include "../monitors/Monitor.h"

class MonitorsManager
{
protected:
  MonitorsManager();
  virtual ~MonitorsManager();

public:
  static __int64 StartMonitor(wchar_t path, bool recursive);
  static bool StopMonitor(__int64 id );

protected:
  Monitor* Create();
  bool Remove( __int64 id );
  __int64 GetId() const;
protected:
  // The file lock
  static std::recursive_mutex _lock;

  // the singleton
  static MonitorsManager* _instance;
  static MonitorsManager* Instance();

protected:
  typedef std::unordered_map<__int64, Monitor*> MonitorMap;
  MonitorMap _monitors;
};
