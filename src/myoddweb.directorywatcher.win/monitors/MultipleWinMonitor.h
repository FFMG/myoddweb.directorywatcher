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
      long long GetEvents(std::vector<Event>& events) const override;

    private:
      /**
       * \brief the current monitors.
       */
      std::vector<Monitor*> _monitors;

      /**
       * \brief get the next available id.
       * \return the next usable id.
       */
      long GetNextId() const;

      /**
       * \brief Create all the sub-requests for a prarent request.
       * \param parent the parent request itselft.
       */
      void CreateMonitors(const Request& parent );

      /**
       * \brief Clear all the current data
       */
      void Delete();
    };
  }
}
