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
    class MultipleWinMonitor :
      public Monitor
    {
    private:
      MultipleWinMonitor(__int64 id, const Request& request, const int depth, const int maxDepth );

    public:
      MultipleWinMonitor(__int64 id, const Request& request);
      virtual ~MultipleWinMonitor();

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
       * \brief Get all the sub folders of a given folder.
       * \param folder the starting folder.
       * \return all the sub-folders, (if any). 
       */
      static std::vector<std::wstring> GetAllSubFolders(const std::wstring& folder);

      /**
       * \brief join 2 parts of a path
       * \param lhs the lhs folder.
       * \param rhs the rhs file/folder
       * \param rhsIsFile if the rhs is a file, then we will not add a trailling back-slash
       * \return all the sub-folders, (if any).
       */
      static std::wstring Join(const std::wstring& lhs, const std::wstring& rhs, bool rhsIsFile );

      /**
       * \brief Check if a given directory is a dot or double dot
       * \param directory the lhs folder.
       * \return if it is a dot directory or not
       */
      static bool IsDot(const std::wstring& directory );

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
