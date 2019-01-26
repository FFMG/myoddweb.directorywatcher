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
#include "Monitor.h"
#include <future>
#include "win/Common.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class WinMonitor : public Monitor
    {
    public:
      WinMonitor(__int64 id, const Request& request);

    protected:
      WinMonitor(__int64 id, const Request& request, unsigned long bufferLength);

    public:
      virtual ~WinMonitor();

      void OnStart() override;
      void OnStop() override;
      void OnGetEvents(std::vector<Event>& events) override;

    private:
      win::Common* _directories;
      win::Common* _files;

      const unsigned long _bufferLength;
    };
  }
}