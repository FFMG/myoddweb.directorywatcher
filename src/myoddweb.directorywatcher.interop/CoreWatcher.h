#pragma once
#include <unordered_map>
#include "FunctionTypes.h"
using namespace System;

class CoreWatcher
{
public:
  CoreWatcher();
  ~CoreWatcher();

public:
  /// <summary>
  /// The path we wish to monitor for changes
  /// </summary>
  /// <param name="path">The path we want to monitor.</param>
  /// <returns>Unique Id used to release/stop monitoring</returns>
  __int64 Monitor(String^ path, bool recursive);

protected:
  void Release();
  bool CreateInstance();
  bool CreateUnmanagedFunctions();
  bool CreateUnmanagedFunction(HINSTANCE hInstance, FunctionTypes procType);
  const FARPROC GetUnmanagedFunction(FunctionTypes procType) const;

private:
  typedef std::unordered_map<FunctionTypes, FARPROC> ProcsFarProc;
  ProcsFarProc _farProcs;

  HINSTANCE _hDll;
};

