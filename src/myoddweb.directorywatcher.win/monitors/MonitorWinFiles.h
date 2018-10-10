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
#include "MonitorWinCommon.h"
#include "../utils/EventAction.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class MonitorWinFiles : public MonitorWinCommon
    {
    public:
      MonitorWinFiles(Monitor& parent, unsigned long bufferLength);

    public:
      virtual ~MonitorWinFiles() = default;

    protected:
      /**
       * Get the notification filter.
       * \return the notification filter
       */
      unsigned long GetNotifyFilter() const override;

      /**
       * \brief check if a given string is a file or a directory.
       * \param action the action we are looking at
       * \param path the file we are checking.
       * \return if the string given is a file or not.
       */
      bool IsFile(ManagedEventAction action, const std::wstring& path) const override;
    };
  }
}
