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
#include <Windows.h>
#include <unordered_map>
#include "Request.h"
#include "FunctionTypes.h"
using namespace System;

class Watcher1
{
public:
  Watcher1();
  ~Watcher1();

public:
  /*
   * \brief start the monitor,
   * \parm the request
   * \return the id of the request added, (or -1).
   */
  __int64 Add( const Request& request );

  /*
   * \brief remove a single request by id.
   * \return success or not
   */
  bool Remove( __int64 id );

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

