//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
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
  // start the monitor,
  __int64 Start(String^ path, bool recursive);

  // stop the monitor
  bool Stop(__int64 id);

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

