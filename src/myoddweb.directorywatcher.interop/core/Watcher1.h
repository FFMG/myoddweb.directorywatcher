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
  bool CreateUnmanagedFunctions();
  bool CreateUnmanagedFunction(HINSTANCE hInstance, FunctionTypes procType);
  const FARPROC GetUnmanagedFunction(FunctionTypes procType) const;

private:
  typedef std::unordered_map<FunctionTypes, FARPROC> ProcsFarProc;
  ProcsFarProc _farProcs;

  HINSTANCE _hDll;

  /**
   * \brief the date on the 01/01/1970 so we can create DateTime from milliseconds.
   */
  const System::DateTime _dateTime1970;
};

