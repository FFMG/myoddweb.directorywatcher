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
   * \param workerPool the worker pool
   * \param request details of the request.
   */
  WinMonitor::WinMonitor(const long long id, threads::WorkerPool& workerPool, const Request& request) :
    WinMonitor(id, id, workerPool, request)
  {
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
    Monitor( id, workerPool, request),
    _directories(nullptr),
    _files(nullptr),
    _bufferLength(bufferLength),
    _parentId( parentId )
  {
  }

  WinMonitor::~WinMonitor() = default;

  /**
   * \brief get the id of the parent, the owner of all the monitors.
   * \return the parent id.
   */
  const long long& WinMonitor::ParentId() const
  {
    return _parentId;
  }

  /**
   * \brief process the collected events add/remove them.
   * \param events the collected events.
   */
  void WinMonitor::OnGetEvents(std::vector<Event*>& events)
  {
    //  nothing to do
  }

  void WinMonitor::OnWorkerStop()
  {
    // we can now stop us.
    Monitor::OnWorkerStop();

    // stop the files and directory
    if (_directories != nullptr)
    {
      _directories->Stop();
    }
    if (_files != nullptr)
    {
      _files->Stop();
    }
  }

  /**
   * \brief called when the worker is ready to start
   *        return false if you do not wish to start the worker.
   */
  bool WinMonitor::OnWorkerStart()
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // create the directories monitor
      _directories = new win::Directories(*this, _bufferLength);

      // add the files as well as the directories to the worker pool.
      if( !_directories->Start() )
      {
        delete _directories;
        _directories = nullptr;
        return false;
      }

      // and then the files monitor.
      _files = new win::Files(*this, _bufferLength);

      if( !_files->Start() )
      {
        _directories->Stop();
        delete _directories;
        _directories = nullptr;

        delete _files;
        _files = nullptr;

        return false;
      }
    
      // all done
      return Monitor::OnWorkerStart();
    }
    catch( ... )
    {
      return false;
    }
  }

  /**
   * \brief Give the worker a chance to do something in the loop
   *        Workers can do _all_ the work at once and simply return false
   *        or if they have a tight look they can return true until they need to come out.
   * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
   * \return true if we want to continue or false if we want to end the thread
   */
  bool WinMonitor::OnWorkerUpdate( const float fElapsedTimeMilliseconds )
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      if (!MustStop())
      {
        _directories->Update();
        _files->Update();
      }
    }
    catch( ... )
    {
      // @todo log exception
    }
    return Monitor::OnWorkerUpdate(fElapsedTimeMilliseconds);
  }

  /**
   * \brief called when the worker has completed
   */
  void WinMonitor::OnWorkerEnd()
  {
    MYODDWEB_PROFILE_FUNCTION();
    Monitor::OnWorkerEnd();

    delete _directories;
    _directories = nullptr;
          
    delete _files;
    _files = nullptr;
  }
}