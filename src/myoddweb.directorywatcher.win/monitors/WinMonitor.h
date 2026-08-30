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
      WinMonitor(long long id, long long parentId, threads::WorkerPool& workerPool, const Request& request, unsigned long bufferLength, bool catchUpOnExistingEntries);

    public:
      WinMonitor(long long id, threads::WorkerPool& workerPool, const Request& request);
      WinMonitor(long long id, long long parentId, threads::WorkerPool& workerPool, const Request& request, bool catchUpOnExistingEntries = false);

      virtual ~WinMonitor();

      WinMonitor() = delete;
      WinMonitor(const WinMonitor&) = delete;
      WinMonitor(WinMonitor&&) = delete;
      const WinMonitor& operator=(const WinMonitor&) = delete;
      WinMonitor&& operator=(WinMonitor&&) = delete;

      void on_get_events(std::vector<event*>& events) override;

      [[nodiscard]]
      const long long& parent_id() const override;

    protected:
      /**
       * \brief the non blocking stop function
       */
      void on_worker_stop() override;

      /**
       * \brief called when the worker is ready to start
       *        return false if you do not wish to start the worker.
       */
      bool on_worker_start() override;

      /**
       * \brief Give the worker a chance to do something in the loop
       *        Workers can do _all_ the work at once and simply return false
       *        or if they have a tight look they can return true until they need to come out.
       * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
       * \return true if we want to continue or false if we want to end the thread
       */
      bool on_worker_update(float fElapsedTimeMilliseconds) override;

      /**
       * \brief called when the worker has completed
       */
      void on_worker_end() override;

    private:
      /**
       * \brief report anything already inside this folder, (recursively, if
       *        this is a recursive watch), as synthetic "Added" events.
       *        Used only for folders discovered while already running,
       *        (see _catchUpOnExistingEntries), to close the gap between a
       *        new sub-folder being detected and its watch actually being
       *        armed, during which real filesystem changes would otherwise
       *        be silently lost. \see https://github.com/FFMG/myoddweb.directorywatcher/issues/20
       */
      void catch_up_on_existing_entries();

      win::Common* _directories;
      win::Common* _files;

      const unsigned long _bufferLength;

      const long long _parentId;

      /**
       * \brief if true, on_worker_start() will scan our folder's existing
       *        contents once the watch is armed and report them as "Added".
       */
      const bool _catchUpOnExistingEntries;
    };
  }
}
