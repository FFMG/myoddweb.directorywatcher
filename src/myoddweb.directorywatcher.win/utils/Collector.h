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
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "EventInformation.h"
#include "Event.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Class that contains and manages all the events.
     */
    class Collector
    {
    public:
      Collector();
      virtual ~Collector();

      void Add(EventAction action, const std::wstring& path, const std::wstring& file);

      /**
       * \brief fill the vector with all the values currently on record.
       * \param events the events we will be filling
       * \return the number of events we found.
       */
      long long GetEvents( std::vector<myoddweb::directorywatcher::Event>& events);
    private:
      /**
       * \brief This counter is used to cleanp the data once we reach a certain number.
       * This value is not the number of items in the collection
       * It is the number of items added since the last time we checked.
       */
      short _internalCounter;

      /**
       * \brief This is the oldest number of ms we want something to be.
       */
      const short _maxAgeMs;

      /**
       * \brief Add an event to the vector and remove older events.
       * \param event
       */
      void AddEventInformation(const EventInformation& event);

      /**
       * \brief the locks so we can add data.
       */
      std::recursive_mutex _lock;

      /**
       * \brief the events list
       */
      typedef std::vector<EventInformation> Events;
      Events _events;

      long long GetTimeMs() const;
      static std::wstring PathCombine(const std::wstring& lhs, const std::wstring& rhs);
    };
  }
}