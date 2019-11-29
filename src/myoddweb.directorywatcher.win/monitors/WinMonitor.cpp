// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "WinMonitor.h"
#include <string>
#include "win/Directories.h"
#include "win/Files.h"

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
    WinMonitor::WinMonitor(const long long id, long long parentId, const Request& request) :
      WinMonitor(id, parentId, request, MAX_BUFFER_SIZE)
    {
    }

    /**
     * \brief Create the Monitor that uses ReadDirectoryChanges
     *        This is the case where the id is the parent id.
     * \param id the unique id of this monitor
     * \param request details of the request.
     */
    WinMonitor::WinMonitor(const long long id, const Request& request) :
      WinMonitor(id, id, request, MAX_BUFFER_SIZE)
    {
    }

    /**
     * \brief Create the Monitor that uses ReadDirectoryChanges
     * \param id the unique id of this monitor
     * \param parentId the id of the owner of this monitor, (top level)
     * \param request details of the request.
     * \param bufferLength the size of the buffer
     */
    WinMonitor::WinMonitor(const long long id, const long long parentId, const Request& request, const unsigned long bufferLength) :
      Monitor(id, request),
      _directories(nullptr),
      _files(nullptr),
      _bufferLength(bufferLength),
      _parentId( parentId )
    {
    }

    WinMonitor::~WinMonitor()
    {
      Stop();
    }

    /**
     * \brief process the collected events add/remove them.
     * \param events the collected events.
     */
    void WinMonitor::OnGetEvents(std::vector<Event*>& events)
    {
      //  nothing to do
    }

    /**
     * \brief Start monitoring
     */
    void WinMonitor::OnStart()
    {
      // start monitoring the directories
      _directories = new win::Directories(*this, _bufferLength);
      _directories->Start();

      // and then the files.
      _files = new win::Files(*this, _bufferLength);
      _files->Start();
    }

    /**
     * \brief Stop monitoring
     */
    void WinMonitor::OnStop()
    {
      if (_directories != nullptr)
      {
        _directories->Stop();
        delete _directories;
      }
      _directories = nullptr;

      if (_files != nullptr)
      {
        _files->Stop();
        delete _files;
      }
      _files = nullptr;
    }

    /**
     * \brief get the id of the parent, the owner of all the monitors.
     * \return the parent id.
     */
    const long long& WinMonitor::ParentId() const
    {
      return _parentId;
    }
  }
}
