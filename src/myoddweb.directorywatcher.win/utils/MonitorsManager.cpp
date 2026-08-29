// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "MonitorsManager.h"
#include "Lock.h"
#include "../utils/Wait.h"
#include "../monitors/Base.h"
#include "../monitors/WinMonitor.h"
#include "../monitors/MultipleWinMonitor.h"
#include "Instrumentor.h"
#include "Logger.h"
#include "LogLevel.h"
#include "Threads/WorkerId.h"

namespace myoddweb:: directorywatcher
{
  MonitorsManager* MonitorsManager::_instance = nullptr;
  MYODDWEB_MUTEX MonitorsManager::_lock;

  MonitorsManager::MonitorsManager() :
    _workersPool( nullptr )
  {
    // create the worker pool
    _workersPool = new threads::WorkerPool( MYODDWEB_WORKERPOOL_THROTTLE );
  }

  MonitorsManager::~MonitorsManager()
  {
    delete _workersPool;
    _workersPool = nullptr;
  }

  /**
   * \brief try and get the current instance of monitor manager.
   * If one does not exist, try and create a new instance
   * if this fails we have much bigger problems.
   * \return the one and only monitor manager.
   */
  MonitorsManager* MonitorsManager::instance()
  {
    if (nullptr != _instance)
    {
      return _instance;
    }

    // lock
    MYODDWEB_LOCK(_lock);

    // check again
    if (nullptr != _instance)
    {
      return _instance;
    }

    // Start the global profiling session.
    MYODDWEB_PROFILE_BEGIN_SESSION( "Monitor Global", "Profile-Global.json" );

    try
    {
      // create a new instance
      _instance = new MonitorsManager();

      // return the instance.
      return _instance;
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log( LogLevel::Panic, L"Caught exception '%hs' trying to create the manager!", e.what());

      return nullptr;
    }
  }

  /**
   * \brief Start a monitor
   * \param request the request being added.
   * \return the id of the monitor we started
   */
  long long MonitorsManager::start(const Request& request)
  {
    MYODDWEB_PROFILE_FUNCTION();
    const auto monitor = instance()->create_and_start(request);
    return monitor->id();
  }

  /**
   * \brief If the monitor manager is ready or not.
   * \return if it is ready or not.
   */
  bool MonitorsManager::ready()
  {
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lock);

    // if we do not have an instance... then we have nothing.
    if (_instance == nullptr)
    {
      return false;
    }

    // if we have no worker pool ... we have nothing.
    if( _instance->_workersPool == nullptr || !_instance->_workersPool->started() )
    {
      return false;
    }

    // yield once
    MYODDWEB_YIELD();

    for( const auto monitor : instance()->_monitors )
    {
      if( !monitor.second->started() )
      {
        return false;
      }
    }

    // if we are here they are all ready
    return true;
  }

  /**
   * \brief Try and remove a monitror by id
   * \param id the id of the monitor we want to stop
   * \return if we managed to remove it or not.
   */
  bool MonitorsManager::stop(const long long id)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      MYODDWEB_LOCK(_lock);

      // if we do not have an instance... then we have nothing.
      if (_instance == nullptr)
      {
        return false;
      }

      // try and remove it.
      const auto result = instance()->stop_and_delete_with_lock(id);

      // delete our instance if we are the last one
      if (instance()->_monitors.empty())
      {
        delete _instance;
        _instance = nullptr;
        MYODDWEB_PROFILE_END_SESSION();
      }
      return result;
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log(id, LogLevel::Panic, L"Caught exception '%hs' trying to stop a monitor!", e.what());

      return false;
    }
  }

  /***
   * \brief Create a monitor instance and add it to the list.
   * \param request the request we are creating
   * \return the value.
   */
  Monitor* MonitorsManager::create_and_add_to_list(const Request& request)
  {
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lock);
    try
    {
      for (;;)
      {
        // try and look for an used id.
        const auto id = WorkerId::next_id();
        if (_monitors.find(id) != _monitors.end())
        {
          // get another id.
          continue;
        }

        // add the logger
        Logger::add(id, request.callback_logger());

        // create the new monitor
        Monitor* monitor;
        if (request.recursive())
        {
          monitor = new MultipleWinMonitor(id, *_workersPool, request);
        }
        else
        {
          monitor = new WinMonitor(id, *_workersPool, request);
        }

        // add it to the ilist
        _monitors[monitor->id()] = monitor;

        // and we are done with it.
        return monitor;
      }
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log(LogLevel::Panic, L"Caught exception '%hs' trying to create a monitor for '%s'!", e.what(), request.path() );

      // something broke while trying to create this monitor.
      return nullptr;
    }
  }

  /***
   * \brief Create a monitor instance and add it to the list.
   * \param request the request we are creating
   * \return the value.
   */
  Monitor* MonitorsManager::create_and_start(const Request& request)
  {
    MYODDWEB_PROFILE_FUNCTION();

    Monitor* monitor = nullptr;
    try
    {
      // create a monitor and then add it to our list.
      monitor = create_and_add_to_list(request);

      // we could not create the monitor for some reason
      if( nullptr == monitor)
      {
        Logger::log(LogLevel::Panic, L"I was unable to create and start a monitor for '%s'!", request.path());
        return nullptr;
      }

      // just add our monitor...
      _workersPool->add( *monitor );

      // and return the monitor we created.
      return monitor;
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log(LogLevel::Panic, L"Caught exception '%hs' trying to create and start the monitor, '%s'!", e.what(), request.path() );

      // exception while trying to start
      // remove the one we just added.
      if (monitor != nullptr)
      {
        MYODDWEB_LOCK(_lock);
        stop_and_delete_with_lock(monitor->id());
      }

      // and return null.
      return nullptr;
    }
  }

  /**
   * \brief stop a monitor and then get rid of it if needed, we will assume we have the lock.
   * \paramn id the id we want to delete.
   * \return false if there was a problem or if it does not exist.
   */
  bool MonitorsManager::stop_and_delete_with_lock(const long long id)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      const auto monitor = _monitors.find(id);
      if (monitor == _monitors.end())
      {
        // does not exist.
        return false;
      }

      // stop everything
      if(threads::WaitResult::complete != _workersPool->stop_and_wait( *monitor->second, MYODDWEB_WAITFOR_WORKER_COMPLETION ))
      {
        Logger::log(LogLevel::Warning, L"Timeout while waiting for worker to complete.");
      }

      try
      {
        // we are about delte this worker so we must make sure that it is complete.
        // this should have happened in the previous call.
        // but if we log a message then it means that we probably have a blocking call somewhere.
        _workersPool->stop_and_wait(*monitor->second, -1 );
        delete monitor->second;
      }
      catch (std::exception& e)
      {
        // log the error
        Logger::log(LogLevel::Panic, L"Caught exception '%hs' trying to free monitor memory!", e.what());
      }
      // remove it
      _monitors.erase(monitor);

      // remove the logger
      Logger::remove(id);

      // we are done
      return true;
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log(id, LogLevel::Panic, L"Caught exception '%hs' trying to stop and delete a monitor!", e.what());
      return false;
    }
  }
}
