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
#include "../utils/Collector.h"
#include "../utils/Request.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class Monitor
    {
    public:
      Monitor(__int64 id, const Request& request);
      virtual ~Monitor();

      __int64 Id() const;
      const std::wstring& Path() const;
      bool Recursive() const;
      Collector& EventsCollector() const;

      /**
       * \brief fill the vector with all the values currently on record.
       * \param events the events we will be filling
       * \return the number of events we found.
       */
      long long GetEvents(std::vector<myoddweb::directorywatcher::Event>& events) const;

    public:
      virtual bool Start() = 0;
      virtual void Stop() = 0;

    protected:
      void AddEvent(EventAction action, const std::wstring& fileName ) const;
      void AddRenameEvent(const std::wstring& newFileName, const std::wstring& oldFilename) const;
      void AddEventError(EventAction action) const;

    private:
      const __int64 _id;
      const Request _request;

      Collector* _eventCollector;
    };
  }
}