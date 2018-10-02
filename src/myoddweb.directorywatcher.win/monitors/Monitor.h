#pragma once
#include <string>

class Monitor
{
public:
  Monitor( __int64 id, const std::wstring& path, bool recursive );
  virtual ~Monitor();

  __int64 Id() const;
  const std::wstring& Path() const;
  bool Recursive() const;

public:
  virtual bool Start() = 0;
  virtual void Stop() = 0;

private:
  const __int64 _id;
  const std::wstring _path;
  const bool _recursive;
};