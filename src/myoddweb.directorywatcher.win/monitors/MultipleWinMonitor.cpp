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
    long long MultipleWinMonitor::GetEvents(std::vector<Event>& events) const
    {
      // if we are stopped or stopping, there is nothing for us to do.
      if (Is(Stopped) || Is(Stopping))
      {
        return 0;
      }

      long long count = 0;
      // get the children events
      count += GetEvents(events, _recursiveChildren);

      // then look for the parent events.
      count += GetEvents(events, _nonRecursiveParents );

      // return the total.
      return count;
    }

    #pragma region Private Functions
    /**
     * \brief Stop all the monitors
     * \param container the vector of monitors.
     */
    void MultipleWinMonitor::Stop(const std::vector<Monitor*>& container)
    {
      // and the monitors.
      for (auto it = container.begin(); it != container.end(); ++it)
      {
        (*it)->Stop();
      }
    }

    /**
     * \brief Start all the monitors
     * \param container the vector of monitors.
     */
    void MultipleWinMonitor::Start(const std::vector<Monitor*>& container)
    {
      // and the monitors.
      for (auto it = container.begin(); it != container.end(); ++it)
      {
        (*it)->Start();
      }
    }

    /**
     * \brief get the events from a given container.
     * \param events where we will be adding the events.
     * \param container where we will be reading the events from.
     */
    long long MultipleWinMonitor::GetEvents(std::vector<Event>& events, const std::vector<Monitor*>& container)
    {
      long long count = 0;
      for (auto it = container.begin(); it != container.end(); ++it)
      {
        try
        {
          // get this directory events
          std::vector<Event> levents;
          const auto lcount = (*it)->GetEvents(levents);

          // if we got nothing we just move on.
          if (lcount == 0)
          {
            continue;
          }

          // add them to our list of events.
          events.insert(events.end(), levents.begin(), levents.end());

          // add count.
          count += lcount;
        }
        catch( ... )
        {
          // @todo we need to log this somewhere.
        }
      }
      return count;
    }

    /**
     * \brief Clear all the current data
     */
    void MultipleWinMonitor::Delete()
    {
      // stop everything
      Monitor::Stop();

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
        // we might as well clear everything now.
        container.clear();
      }
    }

    /**
     * \brief Clear all the parents data
     */
    void DeleteNonRecursiveParents();


    /**
     * \brief get the next available id.
     * \return the next usable id.
     */
    long MultipleWinMonitor::GetNextId() const
    {
      // the id does not really matter, but it will be
      // unique to our list of monitors.
      return static_cast<long>(_recursiveChildren.size() ) + static_cast<long>(_nonRecursiveParents.size());
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

      // if the parent was not recursive
      // then we do not need to go further.
      if( !parent.Recursive )
      {
        _recursiveChildren.push_back( new WinMonitor(id, parent) );
        return;
      }

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