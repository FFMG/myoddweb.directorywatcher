// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "WinMonitor.h"
#include <string>

#include "../utils/Instrumentor.h"
#include "Base.h"
#include "win/Directories.h"
#include "win/Files.h"

namespace myoddweb:: directorywatcher
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
    * \param parentId the id of the parent of this monitor.
    * \param workerPool the worker pool
    * \param request details of the request.
    */
  WinMonitor::WinMonitor(const long long id, const long long parentId, threads::WorkerPool& workerPool, const Request& request) :
    WinMonitor(id, parentId, workerPool, request, MAX_BUFFER_SIZE)
  {
  }
      
  /**
   * \brief Create the Monitor that uses ReadDirectoryChanges
   *        This is the case where the id is the parent id.
   * \param id the unique id of this monitor
   * \param request details of the request.
   */
  WinMonitor::WinMonitor(const long long id, const Request& request) :
    WinMonitor(id, new threads::WorkerPool(), request )
  {
  }

  /**
   * \brief Create the Monitor that uses ReadDirectoryChanges
   *        This is the case where the id is the parent id.
   * \param id the unique id of this monitor
   * \param workerPool the worker pool
   * \param request details of the request.
   */
  WinMonitor::WinMonitor(const long long id, threads::WorkerPool* workerPool, const Request& request) :
    WinMonitor(id, id, *workerPool, request)
  {
    _workerPool = workerPool;
  }

  /**
   * \brief Create the Monitor that uses ReadDirectoryChanges
   * \param id the unique id of this monitor
   * \param parentId the id of the owner of this monitor, (top level)
   * \param workerPool the worker pool
   * \param request details of the request.
   * \param bufferLength the size of the buffer
   */
  WinMonitor::WinMonitor(const long long id, const long long parentId, threads::WorkerPool& workerPool, const Request& request, const unsigned long bufferLength) :
    Monitor(id, workerPool, request),
    _workerPool(nullptr),
    _directories(nullptr),
    _files(nullptr),
    _bufferLength(bufferLength),
    _parentId( parentId )
  {
  }

  WinMonitor::~WinMonitor()
  {
    Stop();

    // clear the worker
    delete _workerPool;
    _workerPool = nullptr;
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
    MYODDWEB_PROFILE_FUNCTION();

    // create the directories monitor
    _directories = new win::Directories(*this, _bufferLength);

    // and then the files monitor.
    _files = new win::Files(*this, _bufferLength);

    // add the files as well as the directories to the worker pool.
    WorkerPool().Add({ _directories, _files });
  }

  /**
   * \brief Stop monitoring
   */
  void WinMonitor::OnStop()
  {
    // stop our worker and wait for it to complete.
    while (threads::WaitResult::complete != WorkerPool().StopAndWait({ _directories, _files }, MYODDWEB_WAITFOR_WORKER_COMPLETION))
    {
      MYODDWEB_YIELD();
      MYODDWEB_OUT("Timeout waiting to complete WinMonitor!\n");
    }

    delete _directories;
    _directories = nullptr;
          
    delete _files;
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
