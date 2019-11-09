// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <process.h>
#include "Files.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace win
    {
      /**
       * \brief Create the Monitor that uses ReadDirectoryChanges
       */
      Files::Files(Monitor& parent, const unsigned long bufferLength) :
        Common(parent, bufferLength)
      {
      }

      /**
       * Get the notification filter.
       * \return the notification filter
       */
      unsigned long Files::GetNotifyFilter() const
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
      bool Files::IsFile(const EventAction action, const std::wstring& path) const
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
            return Common::IsFile(action, path);
          }
        }
        catch (...)
        {
          return false;
        }
      }

    }
  }
}