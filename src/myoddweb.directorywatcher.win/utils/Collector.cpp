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
#include "Collector.h"
#include "Lock.h"

namespace myoddweb
{
  namespace directorywatcher
  {

#define MAX_INTERNAL_VECTOR 128
#define MAX_AGE_MS 5000


    Collector::Collector() :
      _internalCounter(0),
      _maxAgeMs(MAX_AGE_MS)
    {
    }

    Collector::~Collector()
    {
    }

    /**
     * \brief Get the time now in milliseconds since 1970
     * \return the current ms time
     */
    long long Collector::GetMillisecondsNowUtc() const
    {
      // https://en.cppreference.com/w/cpp/chrono/system_clock
      // The epoch of system_clock is unspecified, but most implementations use Unix Time 
      // (i.e., time since 00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970, not counting leap seconds).
      const auto now = std::chrono::system_clock::now();
      const auto nowMs = std::chrono::time_point_cast<std::chrono::milliseconds>(now);

      auto value = nowMs.time_since_epoch();
      return value.count();
    }

    /**
     * \brief Combine 2 parts of a file into a single filename.
     *        we do not validate the path or even if the values make no sense.
     * \param lhs the left hand side
     * \param rhs the right hand side.
     * \return The 2 part connected togeter
     */
    std::wstring Collector::PathCombine(const std::wstring& lhs, const std::wstring& rhs)
    {
      // sanity check, if the lhs.length is 0, then we just return the rhs.
      const auto s = lhs.length();
      if (s == 0)
      {
        return rhs;
      }

      // the two type of separators.
      const auto sep1 = L'/';
      const auto sep2 = L'\\';

      const auto l = lhs[s - 1];
      if (l != sep1 && l != sep2)
      {
#ifdef WIN32
        return lhs + sep2 + rhs;
#else
        return lhs + sep1 + rhs;
#endif
      }
      return lhs + rhs;
    }

    /**
     * \brief Add an action to the collection.
     * \param action the action added
     * \param path the root path, (as given to us in the request.)
     * \param file the file from the path.
     */
    void Collector::Add(const EventAction action, const std::wstring& path, const std::wstring& file)
    {
      try
      {
        // We first create the event outside the lock
        // that way, we only have the lock for the shortest
        // posible amount of time.
        EventInformation e;
        e.action = action;
        e.name = PathCombine(path, file);
        e.timeMillisecondsUtc = GetMillisecondsNowUtc();

        // we can now add the event
        AddEventInformation(e);
      }
      catch (...)
      {
        // something wrong happened
      }
    }

    /**
     * \brief fill the vector with all the values currently on record.
     * \param events the events we will be filling
     * \return the number of events we found.
     */
    long long Collector::GetEvents(std::vector<Event>& events)
    {
      //  quickly make a copy of the current list
      Events clone;
      {
        // copy
        auto guard = Lock(_lock);
        clone = _events;

        // we can now remove all the data we are about to process.
        _events.erase(_events.begin(), _events.end());
      }

      // the lock has been released, we are now working with the clone
      for( auto it = clone.begin(); it != clone.end(); ++it )
      {
        Event e = {};
        e.TimeMillisecondsUtc = (*it).timeMillisecondsUtc;
        e.Path = (*it).name;
        switch ((*it).action)
        {
        case EventAction::Error:
        case EventAction::ErrorMemory:
        case EventAction::ErrorOverflow:
        case EventAction::ErrorAborted:
        case EventAction::ErrorCannotStart:
        case EventAction::ErrorAccess:
        case EventAction::Unknown:
        case EventAction::Added:
        case EventAction::Removed:
        case EventAction::Touched:
          e.Action = static_cast<int>((*it).action);
          break;

        case EventAction::RenamedOld:
        case EventAction::RenamedNew:
          e.Action = static_cast<int>(ManagedEventAction::Renamed);
          break;
        }

        events.push_back(e);
      }
      return events.size();
    }

    /**
     * \brief add an event to the array
     * At regular intervals we will be removing old data.
     * \param event the event we are adding to the vector.
     */
    void Collector::AddEventInformation(const EventInformation& event)
    {
      // we can now get the lock so we can add data.
      // the lock is released automatically.
      auto guard = Lock(_lock);

      // add it.
      _events.push_back(event);

      // update the internal counter.
      ++_internalCounter;

      // do we need to clean up?
      if (_internalCounter <= MAX_INTERNAL_VECTOR)
      {
        return;
      }

      // reset the counter so we can check again later.
      _internalCounter = 0;

      // get the current time.
      const auto ms = GetMillisecondsNowUtc() - MAX_AGE_MS;
      for (;;)
      {
        // get the first iterator.
        const auto it = _events.begin();
        if (it == _events.end())
        {
          // we removed everything
          // so there is nothing else for us to do.
          return;
        }

        if ((*it).timeMillisecondsUtc < ms)
        {
          _events.erase(it);
          continue;
        }

        // all the events are added one after the other
        // so we cannot have an older item
        // that is after begin()
        // so we can jump off now.
        return;
      }
    }
  }
}