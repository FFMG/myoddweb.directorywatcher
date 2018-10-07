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
#include <process.h>
#include "MonitorReadDirectoryChangesDirectories.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Create the Monitor that uses ReadDirectoryChanges
     */
    MonitorReadDirectoryChangesDirectories::MonitorReadDirectoryChangesDirectories(Monitor& parent, unsigned long bufferLength) :
      MonitorReadDirectoryChangesCommon( parent, bufferLength )
    {
    }

    /**
     * Get the notification filter.
     * \return the notification filter
     */
    long long MonitorReadDirectoryChangesDirectories::GetNotifyFilter() const
    {
      // what we are looking for.
      // https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstchangenotificationa
      // https://docs.microsoft.com/en-gb/windows/desktop/api/WinBase/nf-winbase-readdirectorychangesw
      return
        // Any directory-name change in the watched directory or subtree causes a change 
        // notification wait operation to return. 
        // Changes include creating or deleting a directory
        FILE_NOTIFY_CHANGE_DIR_NAME
        ;
    }

    /**
     * \brief check if a given string is a file or a directory.
     * \param path the file we are checking.
     * \return if the string given is a file or not.
     */
    bool MonitorReadDirectoryChangesDirectories::IsFile(const std::wstring& path) const
    {
      // we are the directory monitor
      // so it can never be a file.
      return false;
    }
  }
}
