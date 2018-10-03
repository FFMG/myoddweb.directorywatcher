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
#include <mutex>
#include <unordered_map>
#include "Request.h"
#include "Event.h"
#include "../monitors/Monitor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class MonitorsManager
    {
    protected:
      MonitorsManager();
      virtual ~MonitorsManager();

    public:
      /**
       * \brief Start a monitor
       * \param request the request being added.
       * \return the id of the monitor we started
       */
      static __int64 Start(const Request& request);

      /**
       * \brief Try and remove a monitror by id
       * \param id the id of the monitor we want to stop
       * \return if we managed to remove it or not.
       */
      static bool Stop(__int64 id);

      /**
       * \brief Get the latest events.
       * \param id the id of the monitor we would like the events for.
       * \param events the events we will be getting
       * \return the number of items or -ve in case of an error
       */
      static long long GetEvents(long long id, std::vector<Event>& events);

    protected:
      /**
       * \brief Create a monitor and start monitoring, (given the request).
       * \param request contains the information we need to start the monitoring
       * \return the class item we created.
       */
      Monitor* CreateAndStart(const Request& request);

      /**
       * \brief stop a monitor and then get rid of it.
       * \paramn id the id we want to delete.
       * \return false if there was a problem or if it does not exist.
       */
      bool StopAndDelete(__int64 id);

      /**
       * \brief Get a random id
       * We do not check for colisions, it is up to the caller.
       * \return a random id number.
       */
      static __int64 GetId();

      // The file lock
      static std::recursive_mutex _lock;

      // the singleton
      static MonitorsManager* _instance;
      static MonitorsManager* Instance();

      typedef std::unordered_map<__int64, Monitor*> MonitorMap;
      MonitorMap _monitors;
    };
  }
}