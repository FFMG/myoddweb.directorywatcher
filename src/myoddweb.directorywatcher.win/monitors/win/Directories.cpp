// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <process.h>
#include "Directories.h"

namespace myoddweb:: directorywatcher:: win
{
  /**
   * \brief Create the Monitor that uses ReadDirectoryChanges
   */
  Directories::Directories(Monitor& parent, const unsigned long bufferLength) :
    Common(parent, bufferLength)
  {
  }

  /**
   * Get the notification filter.
   * \return the notification filter
   */
  unsigned long Directories::GetNotifyFilter() const
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
   * \param action the action we are looking at
   * \param path the file we are checking.
   * \return if the string given is a file or not.
   */
  bool Directories::IsFile(const EventAction action, const std::wstring& path) const
  {
    // we are the directory monitor
    // so it can never be a file.
    return false;
  }
}