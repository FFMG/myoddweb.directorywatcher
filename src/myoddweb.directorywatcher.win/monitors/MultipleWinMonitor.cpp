// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "Base.h"
#include "MultipleWinMonitor.h"
#include "../utils/Io.h"
#include "../utils/Lock.h"

#include <algorithm>

#ifdef _DEBUG
#include <cassert>
#endif
#include <execution>

#include "../utils/Instrumentor.h"
#include "../utils/Logger.h"
#include "../utils/LogLevel.h"
#include "../utils/Threads/WorkerId.h"

namespace myoddweb::directorywatcher
{
  MultipleWinMonitor::MultipleWinMonitor(const long long id, threads::WorkerPool& workerPool, const Request& request) :
    Monitor( id, workerPool, request)
  {
    // use a standar monitor for non recursive items.
    if (!request.recursive())
    {
      throw std::invalid_argument("The multiple monitor must be recursive.");
    }

    // try and create the list of monitors.
    create_monitors( _request );
  }

  MultipleWinMonitor::~MultipleWinMonitor() noexcept
  {
    // this function does not throw.
    delete_all();
  }

  /**
   * \brief get the id of the parent, the owner of all the monitors.
   * \return the parent id.
   */
  const long long& MultipleWinMonitor::parent_id() const
  {
    return id();
  }

  /**
   * \brief Check if the monitor and all child monitors are ready.
   * \return If all monitors are ready.
   */
  bool MultipleWinMonitor::ready() const
  {
    if (!Monitor::ready())
    {
      return false;
    }

    MYODDWEB_LOCK(_lock);
    for (const auto& child : _recursiveChildren)
    {
      if (!child->ready())
      {
        return false;
      }
    }
    for (const auto& parent : _nonRecursiveParents)
    {
      if (!parent->ready())
      {
        return false;
      }
    }
    return true;
  }

  /**
   * \brief fill the vector with all the values currently on record.
   * \param events the events we will be filling
   * \return the number of events we found.
   */
  void MultipleWinMonitor::on_get_events(std::vector<event*>& events)
  {
    // now that we have the lock ... check if we have stopped.
    if (!is(State::started))
    {
      return;
    }

    // guard for multiple (re)entry.
    MYODDWEB_LOCK(_lock);

    // get the children events
    const auto childrentEvents = get_and_process_child_events_in_lock();

    // then look for the parent events.
    const auto parentEvents = get_and_process_parent_events_in_lock();

    //  add the parents and the children
    events.insert(events.end(), childrentEvents.begin(), childrentEvents.end());
    events.insert(events.end(), parentEvents.begin(), parentEvents.end());

    // then sort everything by inserted time
    std::sort(events.begin(), events.end(), Collector::sort_by_time_milliseconds_utc);
  }

#pragma region Woker functions
  void MultipleWinMonitor::on_worker_stop()
  {
    Monitor::on_worker_stop();

    // stop the parents
    stop_all(_nonRecursiveParents);

    // and the children
    stop_all(_recursiveChildren);
  }

  /**
   * \brief called when the worker is ready to start
   *        return false if you do not wish to start the worker.
   */
  bool MultipleWinMonitor::on_worker_start()
  {
    try
    {
      Logger::log( id(), LogLevel::Information, L"Started Multiple monitor with '%d' recursive and non recursive monitors", _nonRecursiveParents.size() + _recursiveChildren.size());

      // start the parents
      start_all(_nonRecursiveParents);

      // and the children
      start_all(_recursiveChildren);

      return Monitor::on_worker_start();
    }
    catch ( std::exception& e)
    {
      Logger::log(parent_id(), LogLevel::Error, L"Caught exception '%hs' trying to start the callback!", e.what());
      return false;
    }
  }

  /**
   * \brief Give the worker a chance to do something in the loop
   *        Workers can do _all_ the work at once and simply return false
   *        or if they have a tight look they can return true until they need to come out.
   * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
   * \return true if we want to continue or false if we want to end the thread
   */
  bool MultipleWinMonitor::on_worker_update(float fElapsedTimeMilliseconds)
  {
    return Monitor::on_worker_update( fElapsedTimeMilliseconds );
  }

  /**
   * \brief called when the worker has completed
   */
  void MultipleWinMonitor::on_worker_end()
  {
    MYODDWEB_PROFILE_FUNCTION();
    Monitor::on_worker_end();
  }
#pragma endregion

#pragma region Private Functions
  /**
   * \brief look for a possible child with a matching path.
   * \param path the path we are looking for.
   * \return if we find it, the iterator of the child monitor.
   */
  Monitor* MultipleWinMonitor::find_child_in_lock(const std::wstring& path) const
  {
    for (const auto& child : _recursiveChildren )
    {
      if (child->is_path(path))
      {
        return child;
      }
    }
    return nullptr;
  }

  /**
   * \brief remove all the folders that are no longer being monitored, (complete).
   */
  void MultipleWinMonitor::remove_completed_folders_in_lock()
  {
    for (auto it = _recursiveChildren.begin(); it != _recursiveChildren.end(); ++it)
    {
      // the monitor
      const auto monitor = (*it);

      if (!monitor->completed())
      {
        continue;
      }

      // we are done with this monitor.
      // while we know it is complete, (from the previous check)
      // we are still going to tell the worker pool to do all the required cleanup
      worker_pool().stop_and_wait(*monitor, -1 );
      delete monitor;
      _recursiveChildren.erase(it);

      // then we want to restart
      it = _recursiveChildren.begin();
    }
  }

  /**
   * \brief a folder has been added, process it.
   * \param path the event being processed
   */
  void MultipleWinMonitor::process_added_folder_in_lock(const wchar_t* path)
  {
    if (path == nullptr)
    {
      return;
    }

    // cleanup folders
    remove_completed_folders_in_lock();

    // a folder was added to this path
    // so we have to add this path as a child.
    const auto id = WorkerId::next_id();
    const auto request = Request(path, true, _request.events_callback_rate_milliseconds(), _request.stats_callback_rate_milliseconds() );

    // this folder appeared while we were already running, so its watch will
    // only be armed with a delay; scan its contents once armed so nothing
    // created during that delay is lost.
    // \see https://github.com/FFMG/myoddweb.directorywatcher/issues/20
    const auto child = new WinMonitor(id, parent_id(), worker_pool(), request, true );
    _recursiveChildren.emplace_back(child);

    // add the child.
    worker_pool().add( *child );
  }

  /**
   * \brief a folder has been deleted, process it.
   * \param path the event being processed
   */
  void MultipleWinMonitor::process_deleted_folder_in_lock(const wchar_t* path)
  {
    if (nullptr == path)
    {
      return;
    }

    // cleanup folders
    remove_completed_folders_in_lock();

    // the 'path' folder was removed.
    // so we have to remove it as well as all the child folders.
    // 'cause if it was removed ... then so were the others.
    const auto monitor = find_child_in_lock(path);
    if (monitor == nullptr )
    {
      return;
    }

    // stop it...
    monitor->stop();

    // we do not remove it here.
    // we wait for it to stop in its own thread.
  }

  /**
   * \brief a folder has been renamed, process it.
   * \param path the event being processed
   * \param oldPath the old name being renamed.
   */
  void MultipleWinMonitor::process_renamed_folder_in_lock(const wchar_t* path, const wchar_t* oldPath)
  {
    // add the new one
    process_added_folder_in_lock(path);

    // delete the old one
    process_deleted_folder_in_lock(oldPath);
  }

  /**
   * \brief process the parent events
   * \return events the events we will be adding to
   */
  std::vector<event*> MultipleWinMonitor::get_and_process_parent_events_in_lock()
  {
    // get the events
    std::vector<event*> events;

    // the current events.
    std::vector<event*> levents;
    for ( const auto& monitor : _nonRecursiveParents )
    {
      try
      {
        // if we are stopped or stopping, there is nothing for us to do.
        if (is(State::stopped) || is(State::stopping))
        {
          return events;
        }

        // get this directory events
        if (0 == monitor->get_events(levents))
        {
          continue;
        }

        // by definiton we know that the parents are non-recursive
#ifdef _DEBUG
        assert(!monitor->recursive());
#endif
        // we now need to look for added/deleted paths.
        for ( const auto& levent : levents)
        {
          // we don't care about file events.
          if (levent->IsFile)
          {
            continue;
          }

          // we care about deleted/added folder events.
          switch (static_cast<EventAction>(levent->Action))
          {
          case EventAction::Added:
            process_added_folder_in_lock(levent->Name);
            break;

          case EventAction::Renamed:
            process_renamed_folder_in_lock(levent->Name, levent->OldName);
            break;

          case EventAction::Removed:
            process_deleted_folder_in_lock(levent->Name);
            break;

          default:
            // we don't care...
            break;
          }
        }

        // add them to our list of events.
        events.insert(events.end(), levents.begin(), levents.end());

        // clear the list
        levents.clear();
      }
      catch (...)
      {
        save_current_exception();
      }
    }
    return events;
  }

  /**
   * \brief process the cildren events
   * \return events the events we will be adding to
   */
  std::vector<event*> MultipleWinMonitor::get_and_process_child_events_in_lock() const
  {
    // all the events.
    std::vector<event*> events;
    for ( const auto& monitor : _recursiveChildren)
    {
      const auto& levents = get_events(monitor);
      if (levents.empty())
      {
        continue;
      }
      events.insert(events.end(), levents.begin(), levents.end());
    }
    return events;
  }

  /**
   * \brief process the children events
   * \param monitor the monitor we are getting the events for.
   * \rerturn events the events we will be adding to
   */
  std::vector<event*> MultipleWinMonitor::get_events(Monitor* monitor) const
  {
    try
    {
      // if we are stopped or stopping, there is nothing for us to do.
      if (is(State::stopped) || is(State::stopping))
      {
        return {};
      }

      // the current events.
      std::vector<event*> events;

      // get this directory events
      monitor->get_events(events);

      // add them to our list of events.
      return events;
    }
    catch (...)
    {
      save_current_exception();
    }
    return {};
  }

  /**
   * \briefFunction to call montior functions...
   * \param container the vector of monitors.
   */
   /**
    * \brief Stop all the monitors
    * \param container the vector of monitors.
    */
  void MultipleWinMonitor::stop_all(std::vector<Monitor*>& container) const
  {
    MYODDWEB_PROFILE_FUNCTION();
    for ( const auto& worker : container)
    {
      worker_pool().stop_worker( *worker );
    }
  }

  /**
   * \brief Start all the monitors
   * \param container the vector of monitors.
   */
  void MultipleWinMonitor::start_all(const std::vector<Monitor*>& container) const
  {
    MYODDWEB_PROFILE_FUNCTION();
    for (const auto& worker : container)
    {
      worker_pool().add(*worker);
    }
  }

  /**
   * \brief Clear all the current data
   */
  void MultipleWinMonitor::delete_all() noexcept
  {
    // guard for multiple entry.
    MYODDWEB_LOCK(_lock);

    // delete the children
    delete_in_lock(_recursiveChildren);

    // and the parents
    delete_in_lock(_nonRecursiveParents);
  }

  /**
     * \brief Clear the container data
     * \param container the container we want to clear.
     */
  void MultipleWinMonitor::delete_in_lock(std::vector<Monitor*>& container) const noexcept
  {
    try
    {
      // delete all the monitors.
      for (const auto& monitor : container )
      {
        // if this fires then you might have a problem here
        // because of the way the monitor destructor wait
        // we might deadlock depending when this function was called.
        if(threads::WaitResult::complete != monitor->stop_and_wait(MYODDWEB_WAITFOR_WORKER_COMPLETION) )
        {
          Logger::log(monitor->id(), LogLevel::Warning, L"Trying to dispose of monitor that is not yet complete! We might deadlock.");
        }

        // we are done with this monitor we must now really wait for this worker.
        // to complete so we can remove it from our worker pool
        // this should have completed in the previous call.
        worker_pool().stop_and_wait(*monitor, -1 );
        delete monitor;
      }

      // all done so we can clear all the constructor.
      container.clear();
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' in delete_in_lock", e.what());

      // we might as well clear everything now.
      container.clear();
    }
  }

  /**
   * \brief The total number of items being monitored.
   * \return the total size.
   */
  long MultipleWinMonitor::total_size() const
  {
    // the id does not really matter, but it will be
    // unique to our list of monitors.
    return static_cast<long>(_recursiveChildren.size()) + static_cast<long>(_nonRecursiveParents.size());
  }

  /**
   * \brief Create all the sub-requests for a prarent request.
   * \param parent the parent request itselft.
   */
  void MultipleWinMonitor::create_monitors(const Request& parent )
  {
    // if we are stopping, then we cannot go further.
    if (is(State::stopping))
    {
      return;
    }

    // get the next id.
    const auto id = WorkerId::next_id();

#ifdef _DEBUG
    // this whole class expects recursive requests
    // so we should not be able to have anything
    // other than recursive.
    assert(parent.recursive());
#endif

    // look for all the sub-paths
    const auto subPaths = Io::get_all_sub_folders(parent.path());
    if (subPaths.empty() || total_size() > MYODDWEB_MAX_NUMBER_OF_SUBPATH)
    {
      // we will breach the depth
      _recursiveChildren.push_back(new WinMonitor(id, parent_id(), worker_pool(), parent ));
      return;
    }

    // adding all the sub-paths will not breach the limit.
    // so we can add the parent, but non-recuresive.
    const auto request = Request(parent.path(), false, parent.events_callback_rate_milliseconds(), parent.stats_callback_rate_milliseconds());
    _nonRecursiveParents.emplace_back(new WinMonitor(id, parent_id(), worker_pool(), request ));

    // now try and add all the subpath
    for (const auto& path : subPaths)
    {
      // add one more to the list.
      const auto subRequest = Request(path.c_str(), true, parent.events_callback_rate_milliseconds(), parent.stats_callback_rate_milliseconds());
      create_monitors( subRequest );
    }
  }
#pragma endregion
}
