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
      enum State
      {
        Starting,
        Started,
        Stopping,
        Stopped
      };
      MultipleWinMonitor(__int64 id, const Request& request, const int depth, const int maxDepth );

    public:
      MultipleWinMonitor(__int64 id, const Request& request);
      virtual ~MultipleWinMonitor();

      MultipleWinMonitor& operator=(MultipleWinMonitor&& other) = delete;
      MultipleWinMonitor(MultipleWinMonitor&&) = delete;
      MultipleWinMonitor() = delete;
      MultipleWinMonitor(const MultipleWinMonitor&) = delete;
      MultipleWinMonitor& operator=(const MultipleWinMonitor&) = delete;

      bool Start() override;
      void Stop() override;
      long long GetEvents(std::vector<Event>& events) const override;

    private:
      /**
       * \brief the current monitors.
       */
      std::vector<Monitor*> _monitors;

      /**
       * \brief this is the current state.
       */
      State _state;

      /**
       * \brief return if the current state is the same as the one we are after.
       * \param state the state we are checking against.
       */
      bool Is(State state) const;

      /**
       * \brief get the next available id.
       * \return the next usable id.
       */
      long GetNextId() const;

      /**
       * \brief Create all the sub-requests for a prarent request.
       * \param parent the parent request itselft.
       * \param depth the current depth.
       * \param maxDepth the maximum depth we want to go to.
       */
      void CreateMonitors(const Request& parent, int depth, int maxDepth);

      /**
       * \brief Clear all the current data
       */
      void Delete();
    };
  }
}
