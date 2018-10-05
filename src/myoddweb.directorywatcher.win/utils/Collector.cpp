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
    /**
     * \brief how often we want to check for 'over-full' containers.
     */
    #define MAX_INTERNAL_COUNTER 128
    #define MAX_AGE_MS 5000

    Collector::Collector() :Collector(MAX_INTERNAL_COUNTER, MAX_AGE_MS)
    {
    }

    Collector::Collector(const short maxInternalCounter, const short maxAgeMs) :
      _internalCounter(0),
      _maxInternalCounter(maxInternalCounter),
      _maxAgeMs(maxAgeMs)
    {
    }

    Collector::~Collector() = default;

    /**
     * \brief Get the time now in milliseconds since 1970
     * \return the current ms time
     */
    long long Collector::GetMillisecondsNowUtc()
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

        // we can now add the event to our vector.
        AddEventInformation(e);

        // try and cleanup the internal counter.
        CleanupInternalCounter();
      }
      catch (...)
      {
        // something wrong happened
      }
    }

    /**
     * \brief copy the current content of the events into a local variable.
     * Then erase the current content so we can continue receiving data.
     */
    void Collector::CloneEventsAndEraseCurrent(Events& clone )
    {
      // lock
      auto guard = Lock(_lock);

      // copy
      clone = _events;

      // we can now remove all the data we are about to process.
      _events.erase(_events.begin(), _events.end());

      // we can reset the internal counter.
      // we erased all the data, there is nothing else to do.
      _internalCounter = 0;
    }

    /**
     * \brief fill the vector with all the values currently on record.
     * \param events the events we will be filling
     * \return the number of events we found.
     */
    long long Collector::GetEvents(std::vector<Event>& events)
    {
      // quickly make a copy of the current list
      // and erase the current contents.
      // the lock is released as soon as posible making sure that other threads
      // can keep adding to the list.
      Events clone;
      CloneEventsAndEraseCurrent(clone);

      // the lock has been released, we are now working with the clone

      // go around the data from the newest to the oldest.
      // and we will add them in reverse as well.
      // this is useful to make sure that we remove 'older' dulicates.
      for( auto it = clone.rbegin(); it != clone.rend(); ++it )
      {
        const auto& eventInformation = (*it);

        // look if this is a rename event
        // and if we can add it to a previous rename item.
        if (AddRename(events, eventInformation ))
        {
          continue;
        }

        Event e = {};
        e.TimeMillisecondsUtc = eventInformation.timeMillisecondsUtc;
        e.Path = eventInformation.name;
        e.Action = ConvertEventActionToUnManagedAction(eventInformation.action);
        if (IsOlderDuplicate(events, e))
        {
          // it is an older duplicate
          // so we do not want to add it,
          continue;
        }

        // it is not a duplicate, so we can add it.
        // because we are getting the data in reverse we will add the data
        // in the front, it is older than the previous one.
        events.insert( events.begin(), e);
      }

      // last step is to cleanup all the renames.
      ValidateRenames(events);

      return events.size();
    }

    /**
     * \brief check if the given event is a rename, (or or new)
     * \param source the collection of events we will be looking in
     * \param rename the event we might add to as new or old name.
     * \return true if we added the event.
     */
    bool Collector::AddRename( std::vector<Event>& source, const EventInformation& rename )
    {
      // if we are not processing a rename event, then we might as well go on.
      if( rename.action != EventAction::RenamedOld && rename.action != EventAction::RenamedNew )
      {
        return false;
      }

      Event e;
      e.TimeMillisecondsUtc = rename.timeMillisecondsUtc;
      e.Action = static_cast<int>(ManagedEventAction::Renamed);

      for( auto it = source.rbegin(); it != source.rend(); ++it )
      {
        auto& ce = (*it);
        if ( ce.Action != static_cast<int>(ManagedEventAction::Renamed))
        {
          continue;
        }

        // if they are both complete... then keep looking
        if ( !ce.Extra.empty() && !ce.Path.empty() )
        {
          continue;
        }

        // do we already have an extra or a path?
        // depending on our type?
        // in that case we cannot go any further.
        if (rename.action == EventAction::RenamedOld)
        {
          // the rename action is old rename
          // so the extra has to be empty
          // otherwise we don't know who this belongs to.
          if( ce.Extra.empty() )
          {
            ce.Extra = rename.name;
            return true;
          }

          //  get out so this is added.
          break;
        }

        // the rename action is new rename
        // so the Path has to be empty
        // otherwise we don't know who this belongs to.
        if (ce.Path.empty())
        {
          ce.Path = rename.name;
          return true;
        }
        //  get out so this is added.
        break;
      }

      // if we are here we know we have a rename event, but not a match
      // so we need to create our own item.
      if (rename.action == EventAction::RenamedOld )
      {
        // the action is 'old' rename
        // so the given value has to be the extra
        e.Path = L"";
        e.Extra = rename.name;
      }
      else
      {
        // the action is 'new' rename
        // so the Path is the given name.
        e.Path = rename.name;
        e.Extra = L"";
      }

      // add it at the front.
      source.insert(source.begin(), e);

      // it is a new item
      return true;
    }

    /**
     * \brief go around all the renamed events and look the the ones that are 'invalid'
     * The ones that do not have a new/old name.
     * \param source the collection of events we will be looking in
     */
    void Collector::ValidateRenames(std::vector<Event>& source)
    {
      for (auto it = source.rbegin(); it != source.rend(); ++it)
      {
        auto& e = (*it);
        if (e.Action != static_cast<int>(ManagedEventAction::Renamed))
        {
          continue;
        }

        // no new old path
        if (e.Extra.empty() && !e.Path.empty())
        {
          // so we have a new name, but no old name
          e.Action = static_cast<int>(ManagedEventAction::Added);
        }

        // no new path
        if (e.Path.empty() && !e.Extra.empty() )
        {
          // so we have an old name, but no new name
          e.Path.swap( e.Extra );
          e.Action = static_cast<int>(ManagedEventAction::Removed );
        }

        // both empty
        if (e.Path.empty() && !e.Extra.empty())
        {
          // not sure this is posible
          // so we will turn the event action into an error.
          e.Action = static_cast<int>(ManagedEventAction::Error);
        }
      }
    }

    /**
     * \brief check if the given information already exists in the source
     * \param source the collection of events we will be looking in
     * \param duplicate the event information we want to add.
     * \return if the event information is already in the 'source'
     */
    bool Collector::IsOlderDuplicate(const std::vector<Event>& source, const Event& duplicate)
    {
      for( auto it = source.begin(); it != source.end(); ++it )
      {
        const auto e = (*it);

        // if the actions are not the same, then it is not an older duplicate.
        if( e.Action != duplicate.Action)
        {
          continue;
        }

        if (e.Path == duplicate.Path)
        {
          // they are the same!
          return true;
        }
      }

      // if we made it here, then it is not an older duplicate
      return false;
    }

    /**
     * \brief convert an EventAction to an un-managed IAction
     * so it can be returned to the calling interface.
     * Our EventAction are fairly similar to the Managed IAction, but not all values are the same
     * For example, RenamedOld and RenamedNew are just 'ManagedAction::Renamed'
     */
    int Collector::ConvertEventActionToUnManagedAction(const EventAction& action)
    {
      switch (action)
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
        return static_cast<int>(action);

      case EventAction::RenamedOld:
      case EventAction::RenamedNew:
        return static_cast<int>(ManagedEventAction::Renamed);

      default:
        return static_cast<int>(ManagedEventAction::Unknown);
      }
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
    }

    /**
     * \brief cleanup the vector if our internal counter has being reached.
     */
    void Collector::CleanupInternalCounter()
    {
      // do we need to clean up? First check outside the lock.
      // we don't really need to check the lock here
      // even if that is true a nano second after we check this the next event will clean it
      // remember that this is only a guide as for when to clean up the vector
      if (_internalCounter <= _maxInternalCounter)
      {
        return;
      }

      // lock and check again if we need to do the work.
      // and if not, get out straight away.
      auto guard = Lock(_lock);
      if (_internalCounter <= _maxInternalCounter)
      {
        return;
      }

      // reset the counter so we can check again later.
      _internalCounter = 0;

      // get the current time.
      const auto ms = GetMillisecondsNowUtc() - _maxAgeMs;
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