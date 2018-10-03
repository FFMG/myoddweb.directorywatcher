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
#include "MonitorsManager.h"
#include "Lock.h"
#include "../monitors/MonitorReadDirectoryChanges.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    MonitorsManager* MonitorsManager::_instance = nullptr;
    std::recursive_mutex MonitorsManager::_lock;

    MonitorsManager::MonitorsManager()
    {
      // initialize random seed:
      // The downcast (and potential data loss) doesn't matter 
      // since we're only using it to seed the RNG
      srand(static_cast<unsigned int>(time(nullptr)));
    }

    MonitorsManager::~MonitorsManager() = default;

    /**
     * \brief try and get the current instance of monitor manager.
     * If one does not exist, try and create a new instance
     * if this fails we have much bigger problems.
     * \return the one and only monitor manager.
     */
    MonitorsManager* MonitorsManager::Instance()
    {
      if (nullptr != _instance)
      {
        return _instance;
      }

      // lock
      auto guard = Lock(_lock);

      // check again
      if (nullptr != _instance)
      {
        return _instance;
      }

      try
      {
        // create a new instance
        _instance = new MonitorsManager();

        // return the instance.
        return _instance;
      }
      catch (...)
      {
        return nullptr;
      }
    }

    /**
     * \brief Start a monitor
     * \param request the request being added.
     * \return the id of the monitor we started
     */
    __int64 MonitorsManager::Start(const Request& request)
    {
      const auto monitor = Instance()->CreateAndStart(request);
      return monitor->Id();
    }

    /**
     * \brief Try and remove a monitror by id
     * \return if we managed to remove it or not.
     */
    bool MonitorsManager::Stop(const __int64 id)
    {
      auto guard = Lock(_lock);

      // if we do not have an instance... then we have nothing.
      if (_instance == nullptr)
      {
        return false;
      }

      try
      {
        // try and remove it.
        const auto result = Instance()->StopAndDelete(id);

        // delete our instance if we are the last one
        if (Instance()->_monitors.empty())
        {
          delete _instance;
          _instance = nullptr;
        }
        return result;
      }
      catch (...)
      {
        return false;
      }
    }

    /**
     * \brief Try and get an usued id
     * \return a random id number
     */
    __int64 MonitorsManager::GetId()
    {
      return (static_cast<__int64>(rand()) << (sizeof(int) * 8)) | rand();
    }

    /***
     * \brief Create a monitor instance and add it to the list.
     * \param request the request we are creating
     * \return the value.
     */
    Monitor* MonitorsManager::CreateAndStart(const Request& request)
    {
      auto guard = Lock(_lock);
      try
      {
        for (;;)
        {
          // try and look for an used id.
          auto id = GetId();
          if (_monitors.find(id) != _monitors.end())
          {
            // get another id.
            continue;
          }

          // create the new monitor
          const auto monitor = new MonitorReadDirectoryChanges(id, request);

          // first add it to the list
          _monitors[monitor->Id()] = monitor;

          try
          {
            // and start monitoring for changes.
            monitor->Start();
          }
          catch (...)
          {
            // exception while trying to start
            // remove the one we just added.
            StopAndDelete(monitor->Id());

            // and return null.
            return nullptr;
          }
          return monitor;
        }
      }
      catch (...)
      {
        // something broke while trying to create this monitor.
        return nullptr;
      }
    }

    /**
     * \brief Stop and remove a monitor
     *        We will delete the monitor as well.
     * \param id the item we want to delete
     * \return success or not.
     */
    bool MonitorsManager::StopAndDelete(const __int64 id)
    {
      try
      {
        auto guard = Lock(_lock);
        const auto monitor = _monitors.find(id);
        if (monitor == _monitors.end())
        {
          // does not exist.
          return false;
        }

        // stop everything
        monitor->second->Stop();

        // delete it
        delete monitor->second;

        // remove it
        _monitors.erase(monitor);

        // we are done
        return true;
      }
      catch (...)
      {
        return false;
      }
    }
  }
}