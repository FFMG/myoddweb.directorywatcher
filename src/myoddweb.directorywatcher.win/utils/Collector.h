// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <atomic>
#include <string>
#include <vector>
#include <mutex>

#include "../monitors/Base.h"
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
      explicit Collector(long long maxCleanupAgeMilliseconds);
      ~Collector();

      /**
       * \brief sort events by TimeMillisecondsUtc
       * \param lhs the lhs element we are checking.
       * \param rhs the rhs element we are checking.
       * \return if we need to swap the two items.
       */
      static bool sort_by_time_milliseconds_utc(const event* lhs, const event* rhs);

      void add(EventAction action, const std::wstring& path, const std::wstring& filename, bool isFile, EventError error);
      void add_rename(const std::wstring& path, const std::wstring&newFilename, const std::wstring&oldFilename, bool isFile, EventError error);

      /**
       * \brief fill the vector with all the values currently on record.
       * \param events the events we will be filling
       */
      void get_events( std::vector<event*>& events);

    private:
      void add(EventAction action, const std::wstring& path, const std::wstring& filename, const std::wstring& oldFileName, bool isFile, EventError error);

      /**
       * \brief This is the oldest number of ms we want something to be.
       * It is *only* removed if _maxInternalCounter is reached.
       */
      const long long _maxCleanupAgeMilliseconds;

      /**
       * \brief The next time we want to check for cleanup
       *        We will be using an atomic variable to make sure that it is thread safe.
       */
      std::atomic<long long> _nextCleanupTimeCheck = 0;

      /**
       * \brief cleanup the vector if our internal counter has being reached.
       */
      void cleanup_events();

      /**
       * \brief Add an event to the vector and remove older events.
       * \param event
       */
      void add_event_information(const event_information* event);

      /**
       * \brief the locks so we can add data.
       */
      MYODDWEB_MUTEX _lock;

      /**
       * \brief the events list
       */
      typedef std::vector<const event_information*> EventsInformation;

      /**
       * \brief this is the event that we are _currently adding data to.
       */
      EventsInformation* _currentEvents;

      /**
       * \brief clear all the events information and delete all the data.
       * \param events the data we want to clear.
       */
      static void clear_events(EventsInformation* events);

      /**
       * \brief Get the time now in milliseconds since 1970
       * \return the current ms time
       */
      static long long get_milliseconds_now_utc();

      /**
       * \brief convert an EventAction to an un-managed IAction
       * so it can be returned to the calling interface.
       */
      static int convert_event_action(const EventAction& action);

      /**
       * \brief convert an EventError to an un-managed IError
       * so it can be returned to the calling interface.
       */
      static int convert_event_error(const EventError& error);

      /**
       * \brief check if the given information already exists in the source
       * \param source the collection of events we will be looking in
       * \param duplicate the event information we want to add.
       * \return if the event information is already in the 'source'
       */
      static bool is_older_duplicate(const std::vector<event*>& source, const event& duplicate);

      /**
       * \brief go around all the renamed events and look the the ones that are 'invalid'
       * The ones that do not have a new/old name.
       * \param source the collection of events we will be looking in
       */
      static void validate_renames(std::vector<event*>& source );

      /**
       * \brief copy the current content of the events into a local variable.
       * Then erase the current content so we can continue receiving data.
       * \return the number of items.
       */
      EventsInformation* clone_events_and_erase_current();
    };
  }
}
