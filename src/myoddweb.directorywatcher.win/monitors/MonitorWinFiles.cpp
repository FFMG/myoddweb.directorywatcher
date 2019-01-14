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
#include "MonitorWinFiles.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Create the Monitor that uses ReadDirectoryChanges
     */
    MonitorWinFiles::MonitorWinFiles(const Monitor& parent, unsigned long bufferLength) :
      MonitorWinCommon(parent, bufferLength)
    {
    }

    /**
     * Get the notification filter.
     * \return the notification filter
     */
    unsigned long MonitorWinFiles::GetNotifyFilter() const
    {
      // what we are looking for.
      // https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstchangenotificationa
      // https://docs.microsoft.com/en-gb/windows/desktop/api/WinBase/nf-winbase-readdirectorychangesw
      return
        // Any file name change in the watched directory or subtree causes a change 
        // notification wait operation to return.
        // Changes include renaming, creating, or deleting a file name.
        FILE_NOTIFY_CHANGE_FILE_NAME |

        // Any attribute change in the watched directory or subtree causes
        // a change notification wait operation to return.
        FILE_NOTIFY_CHANGE_ATTRIBUTES |

        // Any file-size change in the watched directory or subtree causes a change 
        // notification wait operation to return. 
        // The operating system detects a change in file size only when the file is written to the disk. 
        // For operating systems that use extensive caching, detection occurs only when the cache is sufficiently flushed.
        FILE_NOTIFY_CHANGE_SIZE |

        // Any change to the last write-time of files in the watched directory or subtree causes a change 
        // notification wait operation to return. The operating system detects a change
        // to the last write-time only when the file is written to the disk. 
        // For operating systems that use extensive caching, detection occurs only when the cache is sufficiently flushed.
        FILE_NOTIFY_CHANGE_LAST_WRITE |

        // Any change to the last access time of files in the watched directory or subtree causes a 
        // change notification wait operation to return.
        FILE_NOTIFY_CHANGE_LAST_ACCESS |

        // Any change to the creation time of files in the watched directory or subtree 
        // causes a change notification wait operation to return.
        FILE_NOTIFY_CHANGE_CREATION |

        // Any security-descriptor change in the watched directory or subtree causes 
        // a change notification wait operation to return.
        FILE_NOTIFY_CHANGE_SECURITY;
    }

    /**
     * \brief check if a given string is a file or a directory.
     * \param action the action we are looking at
     * \param path the file we are checking.
     * \return if the string given is a file or not.
     */
    bool MonitorWinFiles::IsFile(const EventAction action, const std::wstring& path) const
    {
      try
      {
        switch (action)
        {
        case EventAction::Added:
          // Because we are not using FILE_NOTIFY_CHANGE_DIR_NAME any added/removed
          // Notifications _must_ be for a file.
          return true;

        case EventAction::Renamed:
          // Because we are not using FILE_NOTIFY_CHANGE_DIR_NAME any added/removed
          // Notifications _must_ be for a file.
          return true;

        case EventAction::Removed:
          // the file was removed, we cannot double check
          // if it is really a file or not.
          // but we are in the 'file' sections, so we will assume it is.
          return true;

        default:
          return MonitorWinCommon::IsFile(action, path);
        }
      }
      catch (...)
      {
        return false;
      }
    }

  }
}
