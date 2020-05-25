// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <Windows.h>
#include "Monitor.h"
#include "win/Common.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class WinMonitor : public Monitor
    {
      WinMonitor(long long id, threads::WorkerPool* workerPool, const Request& request);

    public:
      WinMonitor(long long id, const Request& request);
      WinMonitor(long long id, long long parentId, threads::WorkerPool& workerPool, const Request& request);

    protected:
      WinMonitor(long long id, long long parentId, threads::WorkerPool& workerPool, const Request& request, unsigned long bufferLength);

    public:
      virtual ~WinMonitor();

      void OnStart() override;
      void OnStop() override;
      void OnGetEvents(std::vector<Event*>& events) override;

      const long long& ParentId() const override;

    private:
      /**
         * \brief the worker pool
         */
      threads::WorkerPool* _workerPool;

      win::Common* _directories;
      win::Common* _files;

      const unsigned long _bufferLength;

      const long long _parentId;
    };
  }
}