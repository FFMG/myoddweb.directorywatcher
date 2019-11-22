// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>
#include "../utils/EventAction.h"
#include "../utils/EventError.h"
#include "../utils/Collector.h"
#include "../utils/Request.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class Monitor
    {
    public:
      Monitor(__int64 id, const Request& request);
      virtual ~Monitor();

      Monitor& operator=(Monitor&& other) = delete;
      Monitor(Monitor&&) = delete;
      Monitor() = delete;
      Monitor(const Monitor&) = delete;
      Monitor& operator=(const Monitor&) = delete;

      __int64 Id() const;
      const wchar_t* Path() const;
      bool Recursive() const;
      Collector& EventsCollector() const;

      /**
       * \brief fill the vector with all the values currently on record.
       * \param events the events we will be filling
       * \return the number of events we found.
       */
      long long GetEvents(std::vector<Event>& events);

      /**
       * \brief check if a given path is the same as the given one.
       * \param maybe the path we are checking against.
       * \return if the given path is the same as our path.
       */
      bool IsPath(const std::wstring& maybe) const;

      virtual void OnGetEvents(std::vector<Event>& events) = 0;
      virtual void OnStart() = 0;
      virtual void OnStop() = 0;

      void AddEvent(EventAction action, const std::wstring& fileName, bool isFile ) const;
      void AddRenameEvent(const std::wstring& newFileName, const std::wstring& oldFilename, bool isFile) const;
      void AddEventError(EventError error) const;

      bool Start();
      void Stop();

    protected:
      /**
       * \brief the unique monitor id.
       */
      const __int64 _id;

      /**
       * \brief the request we used to create the monitor.
       */
      const Request* _request;

      /**
       * \brief the current list of collected events.
       */
      Collector* _eventCollector;

      /**
       * The current monitor state
       */
      enum State
      {
        Starting,
        Started,
        Stopping,
        Stopped
      };

      /**
       * \brief return if the current state is the same as the one we are after.
       * \param state the state we are checking against.
       */
      bool Is(State state) const;

    private:
      /**
       * \brief this is the current state.
       */
      State _state;

      /**
       * \brief the locks so we can add data.
       */
      std::recursive_mutex _lock;
    };
  }
}