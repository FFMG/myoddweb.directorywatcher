#pragma once
#include "Monitor.h"
#include <string>

class MonitorFindFirstChange : public Monitor
{
public:
  MonitorFindFirstChange(__int64 id);
  virtual ~MonitorFindFirstChange();

  void Poll(std::wstring path, bool recursive) const;
};

