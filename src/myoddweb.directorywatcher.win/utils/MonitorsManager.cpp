// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "MonitorsManager.h"
#include "Lock.h"
#include "../monitors/WinMonitor.h"
#include "../monitors/MultipleWinMonitor.h"

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
     * \param callback the callback we will be using
     * \param callbackRateMs how often we want t callback
     * \return the id of the monitor we started
     */
    long long MonitorsManager::Start(const Request& request, EventCallback callback, long long callbackRateMs)
    {
      const auto monitor = Instance()->CreateAndStart(request, callback, callbackRateMs);
      return monitor->Id();
    }

    /**
     * \brief Try and remove a monitror by id
     * \param id the id of the monitor we want to stop
     * \return if we managed to remove it or not.
     */
    bool MonitorsManager::Stop(const long long id)
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
     * \brief Get the latest events.
     * \param id the id of the monitor we would like the events for.
     * \param events the events we will be getting
     * \return the number of items or -ve in case of an error
     */
    long long MonitorsManager::GetEvents(const long long id, std::vector<Event>& events)
    {
      auto guard = Lock(_lock);

      // if we do not have an instance... then we have nothing.
      if (_instance == nullptr)
      {
        return -1;
      }

      try
      {
        // Look for that monitor.
        const auto monitor = Instance()->_monitors.find(id);
        if (monitor == Instance()->_monitors.end())
        {
          // does not exist.
          return -1;
        }

        // the monitor seems to exist
        // so we can now go and get the values from the collector.
        return monitor->second->GetEvents(events);
      }
      catch (...)
      {
        return -1;
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
    Monitor* MonitorsManager::CreateAndddToList(const Request& request)
    {
      auto guard = Lock(_lock);
      try
      {
        for (;;)
        {
          // try and look for an used id.
          const auto id = GetId();
          if (_monitors.find(id) != _monitors.end())
          {
            // get another id.
            continue;
          }

          // create the new monitor
          Monitor* monitor;
          if (request.Recursive)
          {
            monitor = new MultipleWinMonitor(id, request);
          }
          else
          {
            monitor = new WinMonitor(id, request);
          }

          // add it to the ilist
          _monitors[monitor->Id()] = monitor;

          // and we are done with it.
          return monitor;
        }
      }
      catch (...)
      {
        // something broke while trying to create this monitor.
        return nullptr;
      }
    }

    /***
     * \brief Create a monitor instance and add it to the list.
     * \param request the request we are creating
     * \param callback the callback when we have events.
     * \param callbackRateMs how often we want t callback
     * \return the value.
     */
    Monitor* MonitorsManager::CreateAndStart(const Request& request, EventCallback callback, long long callbackRateMs)
    {
      Monitor* monitor = nullptr;
      try
      {
        // create a monitor and then add it to our list.
        monitor = CreateAndddToList(request);

        // we could not create the monitor for some reason
        if( nullptr == monitor)
        {
          // @todo we need to log this here.
          return nullptr;
        }

        // and start monitoring for changes.
        monitor->Start( callback, callbackRateMs );

        // and return the monitor we created.
        return monitor;
      }
      catch (...)
      {
        // exception while trying to start
        // remove the one we just added.
        if (monitor != nullptr)
        {
          StopAndDelete(monitor->Id());
        }

        // and return null.
        return nullptr;
      }
    }

    /**
     * \brief Stop and remove a monitor
     *        We will delete the monitor as well.
     * \param id the item we want to delete
     * \return success or not.
     */
    bool MonitorsManager::StopAndDelete(const long long id)
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
