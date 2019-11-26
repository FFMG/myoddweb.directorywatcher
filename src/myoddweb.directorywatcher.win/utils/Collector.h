// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "EventAction.h"
#include "EventInformation.h"
#include "Event.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Class that contains and manages all the events.
     */
    class Collector final
    {
    public:
      Collector();
      virtual ~Collector();

    private:
      explicit Collector( short maxAgeMs );

    public:
      /**
       * \brief sort events by TimeMillisecondsUtc
       * \param lhs the lhs element we are checking.
       * \param rhs the rhs element we are checking.
       * \return if we need to swap the two items.
       */
      static bool SortByTimeMillisecondsUtc(const Event* lhs, const Event* rhs);

      void Add(EventAction action, const std::wstring& path, const std::wstring& filename, bool isFile, EventError error);
      void AddRename(const std::wstring& path, const std::wstring&newFilename, const std::wstring&oldFilename, bool isFile, EventError error);

      /**
       * \brief fill the vector with all the values currently on record.
       * \param events the events we will be filling
       */
      void GetEvents( std::vector<Event*>& events);

    private:
      void Add(EventAction action, const std::wstring& path, const std::wstring& filename, const std::wstring& oldFileName, bool isFile, EventError error);

      /**
       * \brief This is the oldest number of ms we want something to be.
       * It is *only* removed if _maxInternalCounter is reached.
       */
      const short _maxCleanupAgeMilliseconds;

      /**
       * The next time we want to check for cleanup
       */
      long long _nextCleanupTimeCheck = 0;

      /**
       * \brief cleanup the vector if our internal counter has being reached.
       */
      void CleanupEvents();

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

      /**
       * \brief Get the time now in milliseconds since 1970
       * \return the current ms time
       */
      static long long GetMillisecondsNowUtc();

      /**
       * \brief convert an EventAction to an un-managed IAction
       * so it can be returned to the calling interface.
       */
      static int ConvertEventAction(const EventAction& action);

      /**
       * \brief convert an EventError to an un-managed IError
       * so it can be returned to the calling interface.
       */
      static int ConvertEventError(const EventError& error);

      /**
       * \brief check if the given information already exists in the source
       * \param source the collection of events we will be looking in
       * \param duplicate the event information we want to add.
       * \return if the event information is already in the 'source'
       */
      static bool IsOlderDuplicate(const std::vector<Event*>& source, const Event& duplicate);

      /**
       * \brief go around all the renamed events and look the the ones that are 'invalid'
       * The ones that do not have a new/old name.
       * \param source the collection of events we will be looking in
       */
      static void ValidateRenames(std::vector<Event*>& source );

      /**
       * \brief copy the current content of the events into a local variable.
       * Then erase the current content so we can continue receiving data.
       * \return the number of items.
       */
      long CloneEventsAndEraseCurrent(Events& clone);
    };
  }
}