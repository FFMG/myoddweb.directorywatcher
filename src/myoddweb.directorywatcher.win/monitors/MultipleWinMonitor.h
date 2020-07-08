// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "Monitor.h"
#include "WinMonitor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class MultipleWinMonitor final : public Monitor
    {
    public:
      MultipleWinMonitor(long long id, threads::WorkerPool& workerPool, const Request& request);
      virtual ~MultipleWinMonitor();

      MultipleWinMonitor& operator=(MultipleWinMonitor&& other) = delete;
      MultipleWinMonitor(MultipleWinMonitor&&) = delete;
      MultipleWinMonitor() = delete;
      MultipleWinMonitor(const MultipleWinMonitor&) = delete;
      MultipleWinMonitor& operator=(const MultipleWinMonitor&) = delete;

      void OnGetEvents(std::vector<Event*>& events) override;

      [[nodiscard]]
      const long long& ParentId() const override;

      void OnWorkerStop() override;

    protected:
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
      /**
       * \brief the locks so we can add data.
       */
      MYODDWEB_MUTEX _lock;

      /**
       * \brief the non recursive parents, we will monitor new folder for those.
       */
      std::vector<Monitor*> _nonRecursiveParents;

      /**
       * \brief the child monitors that are recursive, we will not monitor new folders here.
       */
      std::vector<Monitor*> _recursiveChildren;

      /**
       * \brief A running count of Ids
       */
      long _nextId{};

      /**
       * \brief get the next available id.
       * \return the next usable id.
       */
      long GetNextId();

      /**
       * \brief get the next available id.
       * \return the next usable id.
       */
      [[nodiscard]]
      long TotalSize() const;

      /**
       * \brief Create all the sub-requests for a prarent request.
       * \param parent the parent request itselft.
       */
      void CreateMonitors(const Request& parent );

      /**
       * \brief Clear all the current data
       */
      void Delete();

      /**
       * \brief a folder has been deleted, process it.
       * \param path the event being processed
       */
      void ProcessDeletedFolderInLock(const wchar_t* path );

      /**
       * \brief a folder has been added, process it.
       * \param path the event being processed
       */
      void ProcessAddedFolderInLock(const wchar_t* path);

      /**
       * \brief a folder has been renamed, process it.
       * \param path the event being processed
       * \param oldPath the old name being renamed.
       */
      void ProcessRenamedFolderInLock(const wchar_t* path, const wchar_t* oldPath);

      /**
       * \brief remove all the folders that are no longer being monitored, (complete).
       */
      void RemoveCompletedFoldersInLock();

      /**
       * \brief process the parent events
       * \return events the events we will be adding to
       */
      std::vector<Event*> GetAndProcessParentEventsInLock();

      /**
       * \brief process the children events
       * \rerturn events the events we will be adding to
       */
      [[nodiscard]]
      std::vector<Event*> GetAndProcessChildEventsInLock() const;

      /**
       * \brief process the children events
       * \param monitor the monitor we are getting the events for.
       * \rerturn events the events we will be adding to
       */
      std::vector<Event*> GetEvents( Monitor* monitor ) const;

      /**
       * \brief look for a posible child with a matching path.
       * \param path the path we are looking for.
       * \return if we find it, the iterator of the child monitor.
       */
      [[nodiscard]]
      std::vector<Monitor*>::const_iterator FindChildInLock(const std::wstring& path) const;

      /**
       * \brief Clear the container data
       * \param container the container we want to clear.
       */
      void DeleteInLock(std::vector<Monitor*>& container);

      /**
       * \brief Stop all the monitors
       * \param container the vector of monitors.
       */
      void Stop( std::vector<Monitor*>& container) const;

      /**
       * \brief Start all the monitors
       * \param container the vector of monitors.
       */
      void Start(const std::vector<Monitor*>& container) const;
    };
  }
}
