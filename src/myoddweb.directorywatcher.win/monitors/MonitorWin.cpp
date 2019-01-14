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
#include "MonitorWin.h"
#include <string>
#include <process.h>
#include "MonitorWinDirectories.h"
#include "MonitorWinFiles.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief The size of the buffer that is pointed to by the lpBuffer parameter, in bytes.
     * ReadDirectoryChangesW fails with ERROR_INVALID_PARAMETER when the buffer length is greater than 64KB
     * \see https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-readdirectorychangesw
     */
    #define MAX_BUFFER_SIZE (unsigned long)65536

    /**
     * \brief Create the Monitor that uses ReadDirectoryChanges
     * \param id the unique id of this monitor
     * \param request details of the request.
     */
    MonitorWin::MonitorWin( const __int64 id, const Request& request) :
      MonitorWin(id, request, MAX_BUFFER_SIZE )
    {
    }

    /**
     * \brief Create the Monitor that uses ReadDirectoryChanges
     * \param id the unique id of this monitor
     * \param request details of the request.
     * \param bufferLength the size of the buffer
     */
    MonitorWin::MonitorWin( const __int64 id, const Request& request, const unsigned long bufferLength) :
      Monitor(id, request),
      _directories(nullptr),
      _files( nullptr),
      _bufferLength(bufferLength)
    {
    }

    MonitorWin::~MonitorWin()
    {
      MonitorWin::Stop();
    }

    /**
     * \brief Stop monitoring
     */
    bool MonitorWin::Start()
    {
      Stop();

      // start monitoring the directories
      _directories = new MonitorWinDirectories( *this, _bufferLength );
      _directories->Start();

      // and then the files.
      _files = new MonitorWinFiles(*this, _bufferLength);
      _files->Start();

      return true;
    }

    /**
     * \brief Stop monitoring
     */
    void MonitorWin::Stop()
    {
      if (_directories != nullptr)
      {
        _directories->Stop();
        delete _directories;
      }
      if (_files != nullptr)
      {
        _files->Stop();
        delete _files;
      }
      _directories = nullptr;
      _files = nullptr;
    }
  }
}
