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
    std::wstring MultipleWinMonitor::Join(const std::wstring& lhs, const std::wstring& rhs, const bool rhsIsFile)
    {
      const auto ll = lhs.length();
      if (ll > 0)
      {
        const auto c = lhs[ll - 1];
        if (c == L'\\' || c == L'/' )
        {
          return Join(lhs.substr(0, ll - 1), rhs, rhsIsFile);
        }
      }

      const auto lr = rhs.length();
      if (lr > 0)
      {
        const auto c = rhs[0];
        if (c == L'\\' || c == L'/')
        {
          return Join(lhs, rhs.substr(1, lr - 1), rhsIsFile);
        }
      }
      return lhs + L'\\' + rhs + (rhsIsFile ? L"" : L"\\");
    }

    /**
     * \brief Check if a given directory is a dot or double dot
     * \param directory the lhs folder.
     * \return if it is a dot directory or not
     */
    bool MultipleWinMonitor::IsDot(const std::wstring& directory)
    {
      return directory == L"." || directory == L"..";
    }

    /**
     * \brief Get all the sub folders of a given folder.
     * \param folder the starting folder.
     * \return all the sub-folders, (if any).
     */
    std::vector<std::wstring> MultipleWinMonitor::GetAllSubFolders(const std::wstring& folder)
    {
      std::vector<std::wstring> names;
      auto searchPath = Join( folder, L"/*.*", true );
      WIN32_FIND_DATA fd= {};
      const auto hFind = ::FindFirstFile(searchPath.c_str(), &fd);
      if (hFind != INVALID_HANDLE_VALUE) {
        do {
          // read all (real) files in current folder
          // , delete '!' read other 2 default folder . and ..
          if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) 
          {
            if (!IsDot(fd.cFileName))
            {
              names.emplace_back(Join(folder, fd.cFileName, false));
            }
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
     * \brief Create all the sub-requests for a prarent request.
     * \param parent the parent request itselft.
     * \param maxNumberOfChildren the maximum number of children we will allow
     * \return all the requests.
     */
    void MultipleWinMonitor::CreateRequests(const Request& parent, const int maxNumberOfChildren, std::vector<Request>& requests )
    {
      // if the parent was not recursive
      // then we do not need to go further.
      if( !parent.Recursive )
      {
        requests.push_back(parent);
        return;
      }

      // look for all the sub-paths
      const auto subPaths = GetAllSubFolders(parent.Path);
      if( static_cast<int>(subPaths.size()) > maxNumberOfChildren || subPaths.empty())
      {
        // we will breach the number of children
        requests.push_back(parent);
        return;
      }

      // adding all the sub-paths will not breach the limit.
      // so we can add the parent, but non-recuresive.
      requests.push_back({ parent.Path, false });

      // now try and add all the subpath
      for (const auto& path : subPaths)
      {
        const int updatedMaxNumberOfChildren = maxNumberOfChildren - subPaths.size() - requests.size();
        CreateRequests({ path, true }, updatedMaxNumberOfChildren, requests );
      }
    }

    /**
     * \brief create a list of monitors.
     */
    void MultipleWinMonitor::CreateMonitors(const Request& request)
    {
      // (re)create all the requests.
      std::vector<Request> requests;
      CreateRequests(request, 256, requests);

      // then add them all.
      for(const auto& subRequest : requests)
      {
        // the id does not really matter, but it will be
        // unique to our list of monitors.
        const auto id = _monitors.size();

        // then create the monitor and add it to our list.
        _monitors.push_back(new WinMonitor( id , subRequest ));
      }
    }
  }
}