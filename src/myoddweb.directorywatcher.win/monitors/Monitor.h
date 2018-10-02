#pragma once
#include <string>

class Monitor
{
public:
  Monitor( __int64 id );
  virtual ~Monitor();

  __int64 Id() const;

public:
  virtual bool Poll(const std::wstring& path, bool recursive) = 0;
  virtual void Stop() = 0;

private:
  const __int64 _id;
};