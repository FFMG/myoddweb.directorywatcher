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
#include "Base.h"
#include "MultipleWinMonitor.h"
#include "../utils/Io.h"
#include "../utils/Lock.h"

#ifdef _DEBUG
#include <cassert>
#endif

namespace myoddweb
{
  namespace directorywatcher
  {
    MultipleWinMonitor::MultipleWinMonitor(const __int64 id, const Request& request) :
      Monitor(id, request)
    {
      // use a standar monitor for non recursive items.
      if (!request.Recursive)
      {
        throw std::invalid_argument("The multiple monitor must be recursive.");
      }

      // try and create the list of monitors.
      CreateMonitors(_request);
    }

    MultipleWinMonitor::~MultipleWinMonitor()
    {
      Delete();
    }

    /**
     * \brief Start monitoring
     */
    void MultipleWinMonitor::OnStart()
    {
      // guard for multiple entry.
      auto guard = Lock(_lock);

      // start the parents
      Start(_nonRecursiveParents);

      // and the children
      Start(_recursiveChildren);
    }

    /**
     * \brief Start monitoring
     */
    void MultipleWinMonitor::OnStop()
    {
      // guard for multiple entry.
      auto guard = Lock(_lock);

      // stop the parents
      Stop(_nonRecursiveParents);

      // and the children
      Stop(_recursiveChildren);
    }

    /**
     * \brief fill the vector with all the values currently on record.
     * \param events the events we will be filling
     * \return the number of events we found.
     */
    void MultipleWinMonitor::OnGetEvents(std::vector<Event>& events)
    {
      // get the children events
      const auto childrentEvents = GetAndProcessChildEvents();

      // then look for the parent events.
      const auto parentEvents = GetAndProcessParentEvents();

      //  add the parents and the children
      events.insert(events.end(), childrentEvents.begin(), childrentEvents.end());
      events.insert(events.end(), parentEvents.begin(), parentEvents.end() );

      // then sort everything by inserted time
      std::sort(events.begin(), events.end(), Collector::SortByTimeMillisecondsUtc);
    }

    #pragma region Private Functions
    /**
     * \brief look for a posible child with a matching path.
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
    void MultipleWinMonitor::ProcessEventAdded(const std::wstring& path)
    {
      // guard for multiple entry.
      auto guard = Lock(_lock);

      // a folder was added to this path
      // so we have to add this path as a child.
      const auto id = GetNextId();
      auto child = new WinMonitor(id, { path, true });
      _recursiveChildren.push_back(child);
      child->Start();
    }

    /**
     * \brief a folder has been deleted, process it.
     * \param path the event being processed
     */
    void MultipleWinMonitor::ProcessEventDelete(const std::wstring& path)
    {
      // guard for multiple entry.
      auto guard = Lock(_lock);

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
    void MultipleWinMonitor::ProcessEventRenamed(const std::wstring& path, const std::wstring& oldPath)
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
    std::vector<Event> MultipleWinMonitor::GetAndProcessParentEvents()
    {
      // get the events
      std::vector<Event> events;

      // the current events.
      std::vector<Event> levents;
      for (auto it = _nonRecursiveParents.begin(); it != _nonRecursiveParents.end(); ++it)
      {
        try
        {
          // if we are stopped or stopping, there is nothing for us to do.
          if (Is(Stopped) || Is(Stopping))
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
            if( levent.IsFile )
            {
              continue;
            }

            // we care about deleted/added folder events.
            switch (static_cast<EventAction>(levent.Action))
            {
            case EventAction::Added:
              ProcessEventAdded(levent.Name);
              break;

            case EventAction::Renamed:
              ProcessEventRenamed(levent.Name, levent.OldName);
              break;

            case EventAction::Removed:
              ProcessEventAdded(levent.Name);
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
    std::vector<Event> MultipleWinMonitor::GetAndProcessChildEvents() const
    {
      // all the events.
      std::vector<Event> events;

      // the current events.
      std::vector<Event> levents;
      for (auto it = _recursiveChildren.begin(); it != _recursiveChildren.end(); ++it)
      {
        try
        {
          // if we are stopped or stopping, there is nothing for us to do.
          if (Is(Stopped) || Is(Stopping))
          {
            return events;
          }

          // get this directory events
          if (0 == (*it)->GetEvents(levents))
          {
            continue;
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
     * \briefFunction to call montior functions...
     * \param container the vector of monitors.
     * \param function the function we will be calling.
     */
    template <class T, class>
    void MultipleWinMonitor::Do(const std::vector<Monitor*>& container, T&& function)
    {
      const auto numThreads = container.size();
      if (numThreads == 0)
      {
        return;
      }

      const auto ts = new std::thread[numThreads];
      try
      {
        auto current = 0;
        for (auto it = container.begin(); it != container.end(); ++it)
        {
          try
          {
            ts[current++] = std::thread(function, *it);
          }
          catch (...)
          {
            // @todo we need to log this somewhere.
          }
        }

        for (size_t i = 0; i < numThreads; ++i)
        {
          ts[i].join();
        }
      }
      catch (...)
      {
        // @todo we need to log this somewhere.
      }
      delete[] ts;
    }

    /**
     * \brief Stop all the monitors
     * \param container the vector of monitors.
     */
    void MultipleWinMonitor::Stop(const std::vector<Monitor*>& container)
    {
      Do(container, &Monitor::Stop);
    }

    /**
     * \brief Start all the monitors
     * \param container the vector of monitors.
     */
    void MultipleWinMonitor::Start(const std::vector<Monitor*>& container)
    {
      Do(container, &Monitor::Start);
    }

    /**
     * \brief Clear all the current data
     */
    void MultipleWinMonitor::Delete()
    {
      // stop everything
      Monitor::Stop();

      // guard for multiple entry.
      auto guard = Lock(_lock);

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
          delete *it;
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
      if( Is( Stopping) )
      {
        return;
      }

      // get the next id.
      const auto id = GetNextId();

#ifdef _DEBUG
      // this whole class expects recursive requests
      // so we should not be able to have anything
      // other than recursive.
      assert(parent.Recursive);
#endif

      // look for all the sub-paths
      const auto subPaths = Io::GetAllSubFolders(parent.Path);
      if( subPaths.empty() || TotalSize() > MYODDWEB_MAX_NUMBER_OF_SUBPATH )
      {
        // we will breach the depth
        _recursiveChildren.push_back(new WinMonitor(id, parent));
        return;
      }

      // adding all the sub-paths will not breach the limit.
      // so we can add the parent, but non-recuresive.
      _nonRecursiveParents.push_back( new WinMonitor(id, { parent.Path, false }) );

      // now try and add all the subpath
      for (const auto& path : subPaths)
      {
        // add one more to the list.
        CreateMonitors( { path, true } );
      }
    }
    #pragma endregion
  }
}