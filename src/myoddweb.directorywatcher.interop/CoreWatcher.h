#pragma once
#include <unordered_map>
#include "FunctionTypes.h"

class CoreWatcher
{
public:
  CoreWatcher();
  ~CoreWatcher();

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

