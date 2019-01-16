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
#include "MultipleWinMonitor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    MultipleWinMonitor::MultipleWinMonitor(const __int64 id, const Request& request) :
      Monitor(id, request)
    {
    }

    MultipleWinMonitor::~MultipleWinMonitor()
    {
      MultipleWinMonitor::Stop();
    }

    /**
     * \brief Start monitoring
     */
    bool MultipleWinMonitor::Start()
    {
      // sstop and clear everything
      Stop();

      try
      {
        // try and create the list of monitors.
        CreateMonitors( _request );

        // and start them all.
        for (auto it = _monitors.begin(); it != _monitors.end(); ++it)
        {
          (*it)->Start();
        }
      }
      catch(...)
      {
        AddEventError(EventError::CannotStart);
        return false;

      }
      return false;
    }

    /**
     * \brief Start monitoring
     */
    void MultipleWinMonitor::Stop()
    {
      try
      {
        for (auto it = _monitors.begin(); it != _monitors.end(); ++it)
        {
          (*it)->Stop();
          delete *it;
        }
        // all done
        _monitors.clear();
      }
      catch (...)
      {
        // we might as well clear everything now.
        _monitors.clear();
      }
    }

    /**
     * \brief join 2 parts of a path
     * \param lhs the lhs folder.
     * \param rhs the rhs file/folder
     * \param rhsIsFile if the rhs is a file, then we will not add a trailling back-slash
     * \return all the sub-folders, (if any).
     */
    std::wstring MultipleWinMonitor::FolderJoin(const std::wstring& lhs, const std::wstring& rhs, const bool rhsIsFile)
    {
      const auto ll = lhs.length();
      if (ll > 0)
      {
        const auto c = lhs[ll - 1];
        if (c == L'\\' || c == L'/' )
        {
          return FolderJoin(lhs.substr(0, ll - 1), rhs, rhsIsFile);
        }
      }

      const auto lr = rhs.length();
      if (lr > 0)
      {
        const auto c = rhs[0];
        if (c == L'\\' || c == L'/')
        {
          return FolderJoin(lhs, rhs.substr(1, lr - 1), rhsIsFile);
        }
      }
      return lhs + L'\\' + rhs + (rhsIsFile ? L"" : L"\\");
    }

    /**
     * \brief Get all the sub folders of a given folder.
     * \param folder the starting folder.
     * \return all the sub-folders, (if any).
     */
    std::vector<std::wstring> MultipleWinMonitor::GetAllSubFolders(const std::wstring& folder)
    {
      std::vector<std::wstring> names;
      auto searchPath = FolderJoin( folder, L"/*.*", true );
      WIN32_FIND_DATA fd= {};
      const auto hFind = ::FindFirstFile(searchPath.c_str(), &fd);
      if (hFind != INVALID_HANDLE_VALUE) {
        do {
          // read all (real) files in current folder
          // , delete '!' read other 2 default folder . and ..
          if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
            names.emplace_back(FolderJoin( folder, fd.cFileName, false ));
          }
        } while (::FindNextFile(hFind, &fd));
        ::FindClose(hFind);
      }
      return names;
    }

    /**
     * \brief fill the vector with all the values currently on record.
     * \param events the events we will be filling
     * \return the number of events we found.
     */
    long long MultipleWinMonitor::GetEvents(std::vector<Event>& events) const
    {
      long long count = 0;
      for (auto it = _monitors.begin(); it != _monitors.end(); ++it)
      {
        // get this directory events
        std::vector<Event> levents;
        const auto lcount = (*it)->GetEvents(levents);

        // if we got nothing we just move on.
        if( lcount == 0 )
        {
          continue;
        }

        // add them to our list of events.
        events.insert(events.end(), levents.begin(), levents.end() );

        // add count.
        count += lcount;
      }
      return count;
    }

    /**
     * \brief create a list of monitors.
     */
    void MultipleWinMonitor::CreateMonitors(const Request& request)
    {
      // if we are not recursive, no need to go further.
      if( !request.Recursive )
      {
        _monitors.push_back(new WinMonitor(_monitors.size(), request));
        return;
      }

      // so add the non-recursive path
      CreateMonitors({ request.Path, false });

      // and add all the sub-directories.
      const auto subPaths = GetAllSubFolders(request.Path);
      for(const auto& path : subPaths )
      {
        _monitors.push_back(new WinMonitor(_monitors.size(), { path, true }));
      }
    }
  }
}