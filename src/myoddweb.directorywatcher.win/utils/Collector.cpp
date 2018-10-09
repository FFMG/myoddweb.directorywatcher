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
#include <Windows.h>
#include "Collector.h"
#include "Lock.h"
#include "Io.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief how often we want to check for 'over-full' containers.
     */
    #define MAX_AGE_MS 5000

    Collector::Collector() :Collector( MAX_AGE_MS)
    {
    }

    /**
     * \brief 
     * \param maxAgeMs the maximum amount of time we will be keeping an event for.
     */
    Collector::Collector( const short maxAgeMs) :
      _maxCleanupAgeMillisecons( maxAgeMs )
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
     * \brief Add an action to the collection.
     * \param action the action added
     * \param path the root path, (as given to us in the request.)
     * \param filename the file from the path.
     * \param isFile if this is a file or a folder.
     * \param error if there was an error related
     */
    void Collector::Add(const ManagedEventAction action, const std::wstring& path, const std::wstring& filename, bool isFile, ManagedEventError error)
    {
      // just add the action without an old filename.
      Add(action, path, filename, L"", isFile, error );
    }

    /**
     * \brief Add an action to the collection.
     * \param path the root path, (as given to us in the request.)
     * \param newFilename the new file from the path.
     * \param oldFilename the old name of the file.
     * \param isFile if this is a file or a folder.
     * \param error if there is an error related to the rename
     */
    void Collector::AddRename(const std::wstring& path, const std::wstring& newFilename, const std::wstring& oldFilename, bool isFile, ManagedEventError error)
    {
      // just add the action without an old filename.
      Add(ManagedEventAction::Renamed, path, newFilename, oldFilename, isFile, error );
    }

    /**
     * \brief Add an action to the collection.
     * \param action the action added
     * \param path the root path, (as given to us in the request.)
     * \param filename the file from the path.
     * \param oldFileName in the case of a rename event, that value is used.
     * \param isFile if this is a file or a folder.
     * \param error if there was an error related to the action
     */
    void Collector::Add( const ManagedEventAction action, const std::wstring& path, const std::wstring& filename, const std::wstring& oldFileName, const bool isFile, ManagedEventError error)
    {
      try
      {
        // get the combined path.
        const auto combinedPath = filename.empty() ? L"" : Io::Combine(path, filename);

        // We first create the event outside the lock
        // that way, we only have the lock for the shortest
        // posible amount of time.
        EventInformation e;
        e.Action = action;
        e.Error = error;
        e.Name = combinedPath;
        e.OldName = oldFileName.empty() ? L"" : Io::Combine(path, oldFileName);
        e.TimeMillisecondsUtc = GetMillisecondsNowUtc();
        e.IsFile = isFile;

        // we can now add the event to our vector.
        AddEventInformation(e);

        // try and cleanup the events if need be.
        CleanupEvents();
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
      // and we erased all the data, there is nothing else to do.
      _nextCleanupTimeCheck = 0;
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

        Event e = {};
        e.TimeMillisecondsUtc = eventInformation.TimeMillisecondsUtc;
        e.Name = eventInformation.Name;
        e.OldName = eventInformation.OldName;
        e.Action = ConvertEventAction(eventInformation.Action);
        e.Error = ConvertEventError(eventInformation.Error);
        e.IsFile = eventInformation.IsFile;
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
        if (e.OldName.empty() && !e.Name.empty())
        {
          // so we have a new name, but no old name
          e.Action = static_cast<int>(ManagedEventAction::Added);
        }

        // no new path
        if (e.Name.empty() && !e.OldName.empty() )
        {
          // so we have an old name, but no new name
          e.Name.swap( e.OldName );
          e.Action = static_cast<int>(ManagedEventAction::Removed );
        }

        // both empty
        if (e.Name.empty() && !e.OldName.empty())
        {
          // not sure this is posible
          // so we will turn the event action into an error.
          e.Action = static_cast<int>(ManagedEventAction::Unknown);
          e.Error = static_cast<int>(ManagedEventError::NoFileData);
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
        const auto& e = (*it);

        // they must both be of the same type.
        if (e.IsFile != duplicate.IsFile)
        {
          continue;
        }

        // if the actions are not the same, then it is not an older duplicate.
        if( e.Action != duplicate.Action)
        {
          continue;
        }

        if (e.Name == duplicate.Name)
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
     */
    int Collector::ConvertEventAction(const ManagedEventAction& action)
    {
      return static_cast<int>(action);
    }

    /**
     * \brief convert an EventError to an un-managed IError
     * so it can be returned to the calling interface.
     */
    int Collector::ConvertEventError(const ManagedEventError& error)
    {
      return static_cast<int>(error);
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
      if(_nextCleanupTimeCheck == 0 )
      {
        // when we want to check for the next cleanup
        // if the time is zero then we will use the event time + the max time.
        _nextCleanupTimeCheck = event.TimeMillisecondsUtc + _maxCleanupAgeMillisecons;
      }
    }

    /**
     * \brief Check if we need to cleanup the list of events.
     * This is to prevent the list from getting far too large.
     */
    void Collector::CleanupEvents()
    {
      // get the current time
      const auto now = GetMillisecondsNowUtc();

      // do we need to clean up? First check outside the lock.
      // we don't really need to check the lock here
      // even if that is true a nano second after we check this the next event will clean it
      // remember that this is only a guide as for when to clean up the vector
      if (_nextCleanupTimeCheck != 0 && _nextCleanupTimeCheck > now)
      {
        return;
      }

      // lock and check again if we need to do the work.
      // and if not, get out straight away.
      auto guard = Lock(_lock);
      if (_nextCleanupTimeCheck != 0 && _nextCleanupTimeCheck > now)
      {
        return;
      }

      // reset the counter so we can check again later.
      _nextCleanupTimeCheck = 0;

      // get the current time.
      const auto old = now - _maxCleanupAgeMillisecons;
      auto begin = _events.end();
      auto end = _events.end();
      for( auto it = _events.begin();; ++it )
      {
        // get the first iterator.
        if (it == _events.end())
        {
          // we reached the end of the list
          end = it;
          break;
        }

        // is this item newer than our oler time.
        if ((*it).TimeMillisecondsUtc > old)
        {
          // because everything is ordered from 
          // older to newer, there is no point in going any further.
          break;
        }

        // have we already set the begining?
        if ( begin == _events.end() )
        {
          begin = it;
        }
      }

      // do we hae anything to delete?
      if (begin != _events.end())
      {
        _events.erase(begin, end);
      }
    }
  }
}