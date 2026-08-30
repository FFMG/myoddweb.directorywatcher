// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "WinMonitor.h"
#include <string>

#include "../utils/Instrumentor.h"
#include "../utils/Io.h"
#include "../utils/Logger.h"
#include "../utils/LogLevel.h"
#include "win/Directories.h"
#include "win/Files.h"

namespace myoddweb:: directorywatcher
{
  /**
   * \brief The size of the buffer that is pointed to by the lpBuffer parameter, in bytes.
   * ReadDirectoryChangesW fails with ERROR_INVALID_PARAMETER when the buffer length is greater than 64KB
   * \see https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-readdirectorychangesw
   */
  constexpr unsigned long max_buffer_size = 65536;

   /**
    * \brief Create the Monitor that uses ReadDirectoryChanges
    * \param id the unique id of this monitor
    * \param parentId the id of the parent of this monitor.
    * \param workerPool the worker pool
    * \param request details of the request.
    */
  WinMonitor::WinMonitor(const long long id, const long long parentId, threads::WorkerPool& workerPool, const Request& request, const bool catchUpOnExistingEntries) :
    WinMonitor(id, parentId, workerPool, request, max_buffer_size, catchUpOnExistingEntries)
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
   * \param catchUpOnExistingEntries if true, report this folder's existing
   *        contents as "Added" once the watch is armed, (see issue #20).
   */
  WinMonitor::WinMonitor(const long long id, const long long parentId, threads::WorkerPool& workerPool, const Request& request, const unsigned long bufferLength, const bool catchUpOnExistingEntries) :
    Monitor( id, workerPool, request),
    _directories(nullptr),
    _files(nullptr),
    _bufferLength(bufferLength),
    _parentId( parentId ),
    _catchUpOnExistingEntries( catchUpOnExistingEntries )
  {
  }

  WinMonitor::~WinMonitor() = default;

  /**
   * \brief get the id of the parent, the owner of all the monitors.
   * \return the parent id.
   */
  const long long& WinMonitor::parent_id() const
  {
    return _parentId;
  }

  /**
   * \brief process the collected events add/remove them.
   * \param events the collected events.
   */
  void WinMonitor::on_get_events(std::vector<event*>& events)
  {
    //  nothing to do
  }

  void WinMonitor::on_worker_stop()
  {
    // we can now stop us.
    Monitor::on_worker_stop();

    // stop the files and directory
    if (_directories != nullptr)
    {
      _directories->stop();
    }
    if (_files != nullptr)
    {
      _files->stop();
    }
  }

  /**
   * \brief called when the worker is ready to start
   *        return false if you do not wish to start the worker.
   */
  bool WinMonitor::on_worker_start()
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // create the directories monitor
      _directories = new win::Directories(*this, _bufferLength);

      // add the files as well as the directories to the worker pool.
      if( !_directories->start() )
      {
        delete _directories;
        _directories = nullptr;
        return false;
      }

      // and then the files monitor.
      _files = new win::Files(*this, _bufferLength);

      if( !_files->start() )
      {
        _directories->stop();
        delete _directories;
        _directories = nullptr;

        delete _files;
        _files = nullptr;

        return false;
      }

      // the watch is armed now: report anything that was already on disk
      // the instant we started, (or created in the gap before we started),
      // so a bulk copy-paste into a newly-added folder is not silently
      // lost. \see https://github.com/FFMG/myoddweb.directorywatcher/issues/20
      if (_catchUpOnExistingEntries)
      {
        catch_up_on_existing_entries();
      }

      // all done
      return Monitor::on_worker_start();
    }
    catch( ... )
    {
      save_current_exception();
      return false;
    }
  }

  /**
   * \brief report anything already inside this folder as synthetic "Added"
   *        events. \see https://github.com/FFMG/myoddweb.directorywatcher/issues/20
   */
  void WinMonitor::catch_up_on_existing_entries()
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      for (const auto& entry : Io::get_all_files_and_folders(path(), recursive()))
      {
        add_event(EventAction::Added, entry.first, entry.second);
      }
    }
    catch (...)
    {
      // never let a scan failure prevent the monitor from running -- we
      // still have live ReadDirectoryChangesW notifications going forward.
      Logger::log(id(), LogLevel::Warning, L"Unable to complete catch-up scan for: %s", path());
    }
  }

  /**
   * \brief Give the worker a chance to do something in the loop
   *        Workers can do _all_ the work at once and simply return false
   *        or if they have a tight look they can return true until they need to come out.
   * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
   * \return true if we want to continue or false if we want to end the thread
   */
  bool WinMonitor::on_worker_update( const float fElapsedTimeMilliseconds )
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      if (!must_stop())
      {
        _directories->update();
        _files->update();
      }
    }
    catch( ... )
    {
      save_current_exception();
    }
    return Monitor::on_worker_update(fElapsedTimeMilliseconds);
  }

  /**
   * \brief called when the worker has completed
   */
  void WinMonitor::on_worker_end()
  {
    MYODDWEB_PROFILE_FUNCTION();
    Monitor::on_worker_end();

    delete _directories;
    _directories = nullptr;

    delete _files;
    _files = nullptr;
  }
}
