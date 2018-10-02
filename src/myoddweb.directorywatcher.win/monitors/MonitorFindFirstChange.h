#pragma once
#include "Monitor.h"
#include <string>

class MonitorFindFirstChange : public Monitor
{
public:
  MonitorFindFirstChange(__int64 id);
  virtual ~MonitorFindFirstChange();

public:
  bool Poll(const std::wstring& path, bool recursive);
  void Stop();
};

