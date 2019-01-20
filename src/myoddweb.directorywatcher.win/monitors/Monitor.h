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
      Monitor(__int64 id, Request request);
      virtual ~Monitor();

      Monitor& operator=(Monitor&& other) = delete;
      Monitor(Monitor&&) = delete;
      Monitor() = delete;
      Monitor(const Monitor&) = delete;
      Monitor& operator=(const Monitor&) = delete;

      __int64 Id() const;
      const std::wstring& Path() const;
      bool Recursive() const;
      Collector& EventsCollector() const;

      /**
       * \brief fill the vector with all the values currently on record.
       * \param events the events we will be filling
       * \return the number of events we found.
       */
      virtual long long GetEvents(std::vector<Event>& events) const;

      virtual void OnStart() = 0;
      virtual void OnStop() = 0;

      void AddEvent(EventAction action, const std::wstring& fileName, bool isFile ) const;
      void AddRenameEvent(const std::wstring& newFileName, const std::wstring& oldFilename, bool isFile) const;
      void AddEventError(EventError error) const;

      bool Start();
      void Stop();

    protected:
      const __int64 _id;
      const Request _request;

      Collector* _eventCollector;

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