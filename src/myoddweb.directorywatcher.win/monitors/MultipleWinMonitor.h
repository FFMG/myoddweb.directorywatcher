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
#include "Monitor.h"
#include "WinMonitor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class MultipleWinMonitor : public Monitor
    {
    public:
      MultipleWinMonitor(__int64 id, const Request& request);
      virtual ~MultipleWinMonitor();

      MultipleWinMonitor& operator=(MultipleWinMonitor&& other) = delete;
      MultipleWinMonitor(MultipleWinMonitor&&) = delete;
      MultipleWinMonitor() = delete;
      MultipleWinMonitor(const MultipleWinMonitor&) = delete;
      MultipleWinMonitor& operator=(const MultipleWinMonitor&) = delete;

      void OnStart() override;
      void OnStop() override;
      void OnGetEvents(std::vector<Event>& events) override;

    private:
      /**
       * \brief the locks so we can add data.
       */
      std::recursive_mutex _lock;

      /**
       * \brief the non recursive parents, we will monitor new folder for those.
       */
      std::vector<Monitor*> _nonRecursiveParents;

      /**
       * \brief the child monitors that are recursive, we will not monitor new folders here.
       */
      std::vector<Monitor*> _recursiveChildren;

      /**
       * \brief get the next available id.
       * \return the next usable id.
       */
      long GetNextId() const;

      /**
       * \brief get the next available id.
       * \return the next usable id.
       */
      long TotalSize() const;

      /**
       * \brief Create all the sub-requests for a prarent request.
       * \param parent the parent request itselft.
       */
      void CreateMonitors(const Request& parent );

      /**
       * \brief Clear all the current data
       */
      void Delete();

      /**
       * \brief a folder has been deleted, process it.
       * \param path the event being processed
       */
      void ProcessEventDelete(const std::wstring& path );

      /**
       * \brief a folder has been added, process it.
       * \param path the event being processed
       */
      void ProcessEventAdded(const std::wstring& path);

      /**
       * \brief a folder has been renamed, process it.
       * \param path the event being processed
       * \param oldPath the old name being renamed.
       */
      void ProcessEventRenamed(const std::wstring& path, const std::wstring& oldPath);

      /**
       * \brief process the parent events
       * \return events the events we will be adding to
       */
      std::vector<Event> GetAndProcessParentEvents();

      /**
       * \brief process the cildren events
       * \rerturn events the events we will be adding to
       */
      std::vector<Event> GetAndProcessChildEvents() const;

      /**
       * \brief look for a posible child with a matching path.
       * \param path the path we are looking for.
       * \return if we find it, the iterator of the child monitor.
       */
      std::vector<Monitor*>::const_iterator FindChild(const std::wstring& path) const;

      /**
       * \brief Clear the container data
       * \param container the container we want to clear.
       */
      static void Delete(std::vector<Monitor*>& container);

      /**
       * \brief get the events from a given container.
       * \param events where we will be adding the events.
       * \param container where we will be reading the events from.
       */
      static void GetEvents(std::vector<Event>& events, const std::vector<Monitor*>& container);

      /**
       * \brief Stop all the monitors
       * \param container the vector of monitors.
       */
      static void Stop(const std::vector<Monitor*>& container);

      /**
       * \brief Start all the monitors
       * \param container the vector of monitors.
       */
      static void Start(const std::vector<Monitor*>& container);

      /**
       * \briefFunction to call montior functions...
       * \param container the vector of monitors.
       * \param function the function we will be calling.
       */
      template<class T,
        class = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::thread>>>
        static void Do(const std::vector<Monitor*>& container, T&& function );
    };
  }
}
