// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>
#include "../utils/EventAction.h"
#include "../utils/EventError.h"
#include "../utils/Collector.h"
#include "../utils/Request.h"
#include "../utils/Threads/WorkerPool.h"
#include "EventsPublisher.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class Monitor : public threads::Worker
    {
    public:
      Monitor( __int64 id, threads::WorkerPool& workerPool, const Request& request);
      virtual ~Monitor();

      Monitor& operator=(Monitor&& other) = delete;
      Monitor(Monitor&&) = delete;
      Monitor() = delete;
      Monitor(const Monitor&) = delete;
      Monitor& operator=(const Monitor&) = delete;

      [[nodiscard]]
      const long long& Id() const;
      [[nodiscard]]
      const wchar_t* Path() const;
      [[nodiscard]]
      bool Recursive() const;
      [[nodiscard]]
      const Collector& EventsCollector() const;

      /**
       * \brief check if a given path is the same as the given one.
       * \param maybe the path we are checking against.
       * \return if the given path is the same as our path.
       */
      [[nodiscard]]
      bool IsPath(const std::wstring& maybe) const;

      /**
       * \brief fill the vector with all the values currently on record.
       * \param events the events we will be filling
       * \return the number of events we found.
       */
      long long GetEvents(std::vector<Event*>& events);

      /**
       * \brief Add an event to our current log.
       * \param action the action that was performed, (added, deleted and so on)
       * \param fileName the name of the file/directory
       * \param isFile if it is a file or not
       */
      void AddEvent(EventAction action, const std::wstring& fileName, bool isFile );

      /**
       * \brief Add an event to our current log.
       * \param newFileName the new name of the file/directory
       * \param oldFilename the previous name
       * \param isFile if this is a file or not.
       */
      void AddRenameEvent(const std::wstring& newFileName, const std::wstring& oldFilename, bool isFile);

      /**
       * \brief add an event error to the queue
       * \param error the error event being added
       */
      void AddEventError(EventError error);

      /**
       * \brief get the worker pool
       */
      [[nodiscard]]
      threads::WorkerPool& WorkerPool() const
      {
        return _workerPool;
      }

    protected:
      /**
       * \brief called when the worker is ready to start
       *        return false if you do not wish to start the worker.
       */
      bool OnWorkerStart() override;

      /**
       * \brief stop the worker
       */
      void OnWorkerStop() override;

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

      /**
       * \brief the unique monitor id.
       */
      const long long _id;

      /**
       * \brief the worker pool
       */
      threads::WorkerPool& _workerPool;

      /**
       * \brief the request we used to create the monitor.
       */
      const Request* _request;

      /**
       * \brief the current list of collected events.
       */
      Collector _eventCollector;

      /**
       * \brief how often we want to check for new events.
       */
      EventsPublisher* _publisher;

      /**
       * \brief Start the callback timer so we can publish events.
       */
      void StartEventsPublisher();

      virtual void OnGetEvents(std::vector<Event*>& events) = 0;

      [[nodiscard]]
      virtual const long long& ParentId() const = 0;
    };
  }
}
