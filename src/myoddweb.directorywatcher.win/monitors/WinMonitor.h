// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
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
      WinMonitor(long long id, long long parentId, const Request& request);
      WinMonitor(long long id, const Request& request);

    protected:
      WinMonitor(long long id, long long parentId, const Request& request, unsigned long bufferLength);

    public:
      virtual ~WinMonitor();

      void OnStart() override;
      void OnStop() override;
      void OnGetEvents(std::vector<Event*>& events) override;

      const long long& ParentId() const override;

    private:
      win::Common* _directories;
      win::Common* _files;

      const unsigned long _bufferLength;

      const long long _parentId;
    };
  }
}