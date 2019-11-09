// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <unordered_map>
#include "Request.h"
#include "Event.h"
#include "FunctionTypes.h"

using namespace System;
using namespace System::Collections::Generic;

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
  long long Start(myoddweb::directorywatcher::interfaces::IRequest^ request );

  /*
   * \brief remove a single request by id.
   * \return success or not
   */
  bool Stop( long long id );

  /**
   * \brief Get the latest available events.
   * \param id the id we are looking for.
   * \param events out parameter witht he list of events.
   * \return the number of items returned.
   */
  long long GetEvents(long long id, IList<myoddweb::directorywatcher::interfaces::IEvent^> ^% events );

protected:
  void Release();
  bool CreateInstance();
  bool CreateInstanceFromLocalFolder( String^ core );
  bool CreateInstanceFromPlatformFolder(String^ core);
  bool CreateUnmanagedFunctions();
  bool CreateUnmanagedFunction(HINSTANCE hInstance, FunctionTypes procType);
  const FARPROC GetUnmanagedFunction(FunctionTypes procType) const;

private:
  typedef std::unordered_map<FunctionTypes, FARPROC> ProcsFarProc;
  ProcsFarProc _farProcs;

  HINSTANCE _hDll;
};

