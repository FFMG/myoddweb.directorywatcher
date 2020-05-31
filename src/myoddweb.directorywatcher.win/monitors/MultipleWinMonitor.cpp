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

namespace myoddweb::directorywatcher
{
  MultipleWinMonitor::MultipleWinMonitor(const long long id, threads::WorkerPool& workerPool, const Request& request) :
    Monitor(id, workerPool, request)
  {
    // use a standar monitor for non recursive items.
    if (!request.Recursive())
    {
      throw std::invalid_argument("The multiple monitor must be recursive.");
    }

    // try and create the list of monitors.
    CreateMonitors(*_request );
  }

  MultipleWinMonitor::~MultipleWinMonitor()
  {
    Delete();
  }

  /**
   * \brief get the id of the parent, the owner of all the monitors.
   * \return the parent id.
   */
  const long long& MultipleWinMonitor::ParentId() const
  {
    return Id();
  }

  /**
   * \brief fill the vector with all the values currently on record.
   * \param events the events we will be filling
   * \return the number of events we found.
   */
  void MultipleWinMonitor::OnGetEvents(std::vector<Event*>& events)
  {
    // now that we have the lock ... check if we have stopped.
    if (!Is(State::started))
    {
      return;
    }

    // guard for multiple (re)entry.
    MYODDWEB_LOCK(_lock);

    // get the children events
    const auto childrentEvents = GetAndProcessChildEventsInLock();

    // then look for the parent events.
    const auto parentEvents = GetAndProcessParentEventsInLock();

    //  add the parents and the children
    events.insert(events.end(), childrentEvents.begin(), childrentEvents.end());
    events.insert(events.end(), parentEvents.begin(), parentEvents.end());

    // then sort everything by inserted time
    std::sort(events.begin(), events.end(), Collector::SortByTimeMillisecondsUtc);
  }

#pragma region Woker functions
  void MultipleWinMonitor::OnStop()
  {
    Monitor::OnStop();

    // stop the parents
    Stop(_nonRecursiveParents);

    // and the children
    Stop(_recursiveChildren);
  }

  /**
   * \brief called when the worker is ready to start
   *        return false if you do not wish to start the worker.
   */
  bool MultipleWinMonitor::OnWorkerStart()
  {
    try
    {
      // start the parents
      Start(_nonRecursiveParents);

      // and the children
      Start(_recursiveChildren);

      return Monitor::OnWorkerStart();
    }
    catch( ... )
    {
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
  bool MultipleWinMonitor::OnWorkerUpdate(float fElapsedTimeMilliseconds)
  {
    return Monitor::OnWorkerUpdate( fElapsedTimeMilliseconds );
  }

  /**
   * \brief called when the worker has completed
   */
  void MultipleWinMonitor::OnWorkerEnd()
  {
    Monitor::OnWorkerEnd();
  }
#pragma endregion

#pragma region Private Functions
  /**
   * \brief look for a possible child with a matching path.
   * \param path the path we are looking for.
   * \return if we find it, the iterator of the child monitor.
   */
  std::vector<Monitor*>::const_iterator MultipleWinMonitor::FindChild(const std::wstring& path) const
  {
    for (auto child = _recursiveChildren.begin();
      child != _recursiveChildren.end();
      ++child)
    {
      if ((*child)->IsPath(path))
      {
        return child;
      }
    }
    return _recursiveChildren.end();
  }

  /**
   * \brief a folder has been added, process it.
   * \param path the event being processed
   */
  void MultipleWinMonitor::ProcessEventAdded(const wchar_t* path)
  {

    if (path == nullptr)
    {
      return;
    }

    // guard for multiple entry.
    MYODDWEB_LOCK(_lock);

    // a folder was added to this path
    // so we have to add this path as a child.
    const auto id = GetNextId();
    const auto request = new Request(path, true, nullptr, 0);
    const auto child = new WinMonitor(id, ParentId(), WorkerPool(), *request );
    _recursiveChildren.push_back(child); 

    // add the child.
    WorkerPool().Add( *child );

    delete request;
  }

  /**
   * \brief a folder has been deleted, process it.
   * \param path the event being processed
   */
  void MultipleWinMonitor::ProcessEventDelete(const wchar_t* path)
  {
    if (nullptr == path)
    {
      return;
    }

    // guard for multiple entry.
    MYODDWEB_LOCK(_lock);

    // the 'path' folder was removed.
    // so we have to remove it as well as all the child folders.
    // 'cause if it was removed ... then so were the others.
    const auto child = FindChild(path);
    if (child == _recursiveChildren.end())
    {
      return;
    }

    // stop it...
    (*child)->Stop();

    // delete it
    delete (*child);

    // remove it from the list.
    _recursiveChildren.erase(child);
  }

  /**
   * \brief a folder has been renamed, process it.
   * \param path the event being processed
   * \param oldPath the old name being renamed.
   */
  void MultipleWinMonitor::ProcessEventRenamed(const wchar_t* path, const wchar_t* oldPath)
  {
    // add the new one
    ProcessEventAdded(path);

    // delete the old one
    ProcessEventDelete(oldPath);
  }

  /**
   * \brief process the parent events
   * \return events the events we will be adding to
   */
  std::vector<Event*> MultipleWinMonitor::GetAndProcessParentEventsInLock()
  {
    // get the events
    std::vector<Event*> events;

    // the current events.
    std::vector<Event*> levents;
    for (auto it = _nonRecursiveParents.begin(); it != _nonRecursiveParents.end(); ++it)
    {
      try
      {
        // if we are stopped or stopping, there is nothing for us to do.
        if (Is(State::stopped) || Is(State::stopping))
        {
          return events;
        }

        // the monitor
        auto& monitor = *(*it);

        // get this directory events
        if (0 == monitor.GetEvents(levents))
        {
          continue;
        }

        // by definiton we know that the parents are non-recursive
#ifdef _DEBUG
        assert(!monitor.Recursive());
#endif
        // we now need to look for added/deleted paths.
        for (auto levent : levents)
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
            ProcessEventAdded(levent->Name);
            break;

          case EventAction::Renamed:
            ProcessEventRenamed(levent->Name, levent->OldName);
            break;

          case EventAction::Removed:
            ProcessEventAdded(levent->Name);
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
        // @todo we need to log this somewhere.
      }
    }
    return events;
  }

  /**
   * \brief process the cildren events
   * \return events the events we will be adding to
   */
  std::vector<Event*> MultipleWinMonitor::GetAndProcessChildEventsInLock() const
  {
    // all the events.
    std::vector<Event*> events;
    for (auto monitor : _recursiveChildren)
    {
      const auto levents = GetEvents(monitor);
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
  std::vector<Event*> MultipleWinMonitor::GetEvents(Monitor* monitor) const
  {
    try
    {
      // if we are stopped or stopping, there is nothing for us to do.
      if (Is(State::stopped) || Is(State::stopping))
      {
        return {};
      }

      // the current events.
      std::vector<Event*> events;

      // get this directory events
      monitor->GetEvents(events);

      // add them to our list of events.
      return events;
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
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
  void MultipleWinMonitor::Stop(std::vector<Monitor*>& container) const
  {
    MYODDWEB_PROFILE_FUNCTION();
    WorkerPool().Stop( {container.begin(), container.end()});
  }

  /**
   * \brief Start all the monitors
   * \param container the vector of monitors.
   */
  void MultipleWinMonitor::Start(const std::vector<Monitor*>& container) const
  {
    MYODDWEB_PROFILE_FUNCTION();
    WorkerPool().Add({ container.begin(), container.end() });
  }

  /**
   * \brief Clear all the current data
   */
  void MultipleWinMonitor::Delete()
  {
    // guard for multiple entry.
    MYODDWEB_LOCK(_lock);

    // delete the children
    Delete(_recursiveChildren);

    // and the parents
    Delete(_nonRecursiveParents);
  }

  /**
     * \brief Clear the container data
     * \param container the container we want to clear.
     */
  void MultipleWinMonitor::Delete(std::vector<Monitor*>& container)
  {
    try
    {
      // delete all the monitors.
      for (auto it = container.begin(); it != container.end(); ++it)
      {
        delete* it;
      }

      // all done
      container.clear();
    }
    catch (...)
    {
      // @todo we need to log this.

      // we might as well clear everything now.
      container.clear();
    }
  }

  /**
   * \brief get the next available id.
   * \return the next usable id.
   */
  long MultipleWinMonitor::GetNextId()
  {
    // get the next id and increase the number to make sure it is not used again.
    return _nextId++;
  }

  /**
   * \brief The total number of items being monitored.
   * \return the total size.
   */
  long MultipleWinMonitor::TotalSize() const
  {
    // the id does not really matter, but it will be
    // unique to our list of monitors.
    return static_cast<long>(_recursiveChildren.size()) + static_cast<long>(_nonRecursiveParents.size());
  }

  /**
   * \brief Create all the sub-requests for a prarent request.
   * \param parent the parent request itselft.
   */
  void MultipleWinMonitor::CreateMonitors(const Request& parent )
  {
    // if we are stopping, then we cannot go further.
    if (Is(State::stopping))
    {
      return;
    }

    // get the next id.
    const auto id = GetNextId();

#ifdef _DEBUG
    // this whole class expects recursive requests
    // so we should not be able to have anything
    // other than recursive.
    assert(parent.Recursive());
#endif

    // look for all the sub-paths
    const auto subPaths = Io::GetAllSubFolders(parent.Path());
    if (subPaths.empty() || TotalSize() > MYODDWEB_MAX_NUMBER_OF_SUBPATH)
    {
      // we will breach the depth
      _recursiveChildren.push_back(new WinMonitor(id, ParentId(), WorkerPool(), parent ));
      return;
    }
    
    // adding all the sub-paths will not breach the limit.
    // so we can add the parent, but non-recuresive.
    const auto request = new Request(parent.Path(), false, nullptr, 0);
    _nonRecursiveParents.push_back(new WinMonitor(id, ParentId(), WorkerPool(), *request ));

    // now try and add all the subpath
    for (const auto& path : subPaths)
    {
      // add one more to the list.
      const auto subRequest = new Request(path.c_str(), true, nullptr, 0);
      CreateMonitors(*subRequest );
      delete subRequest;
    }

    // clean up the request.
    delete request;
  }
#pragma endregion
}
