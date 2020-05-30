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
    class WinMonitor final : public Monitor
    {
    protected:
      WinMonitor(long long id, long long parentId, threads::WorkerPool& workerPool, const Request& request, unsigned long bufferLength);

    public:
      WinMonitor(long long id, threads::WorkerPool& workerPool, const Request& request);
      WinMonitor(long long id, long long parentId, threads::WorkerPool& workerPool, const Request& request);

      virtual ~WinMonitor();

      WinMonitor() = delete;
      WinMonitor(const WinMonitor&) = delete;
      WinMonitor(WinMonitor&&) = delete;
      const WinMonitor& operator=(const WinMonitor&) = delete;
      WinMonitor&& operator=(WinMonitor&&) = delete;

      void OnGetEvents(std::vector<Event*>& events) override;

      [[nodiscard]]
      const long long& ParentId() const override;

    protected:
      /**
       * \brief the non blocking stop function
       */
      void OnStop() override;

      /**
       * \brief called when the worker is ready to start
       *        return false if you do not wish to start the worker.
       */
      bool OnWorkerStart() override;

      /**
       * \brief Give the worker a chance to do something in the loop
       *        Workers can do _all_ the work at once and simply return false
       *        or if they have a tight look they can return true until they need to come out.
       * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
       * \return true if we want to continue or false if we want to end the thread
       */
      bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override;

      /**
       * \brief called when the worker has completed
       */
      void OnWorkerEnd() override;

    private:
      win::Common* _directories;
      win::Common* _files;

      const unsigned long _bufferLength;

      const long long _parentId;
    };
  }
}