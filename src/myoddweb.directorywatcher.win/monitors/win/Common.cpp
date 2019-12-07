// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <process.h>
#include <Windows.h>
#include "Common.h"
#include "../Base.h"
#include "../../utils/Io.h"
#include "../../utils/EventError.h"
#include "../../utils/MonitorsManager.h"
#include "../../utils/Instrumentor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace win
    {
      /**
       * \brief Create the Monitor that uses ReadDirectoryChanges
       */
      Common::Common(
        Monitor& parent,
        const unsigned long bufferLength
      ) :
        _mustStop(false),
        _data(nullptr),
        _parent(parent),
        _future(nullptr),
        _function(nullptr),
        _bufferLength(bufferLength)
      {
      }

      Common::~Common()
      {
        Common::Stop();
      }

      /**
       * \brief https://docs.microsoft.com/en-gb/windows/desktop/api/winbase/nf-winbase-readdirectorychangesexw
       * \return success or not.
       */
      bool Common::Start()
      {
        MYODDWEB_PROFILE_FUNCTION();

        // close everything
        Stop();

        // we can alow things to run now
        _mustStop = false;

        // start the worker thread
        // in turn it will start the reading.
        StartWorkerThread();

        return true;
      }

      /**
       * \brief Close all the handles, delete pointers and reset all the values.
       */
      void Common::Stop()
      {
        // tell everybody to stop...
        _mustStop = true;

        // stop and wait for the buffer to complete.
        // we have to wait first otherwise the next step
        // will reset the buffer and cause posible issues.
        StopAndResetThread();

        // clear the data.
        delete _data;
        _data = nullptr;
      }

      /**
       * \brief Stop the worker thread, wait for it to complete and then delete it.
       */
      void Common::StopAndResetThread()
      {
        MYODDWEB_PROFILE_FUNCTION();

        // tell everybody to stop...
        _mustStop = true;

        // wait for the thread to complete.
        if (_future != nullptr)
        {
          (*_future).wait();
        }

        // the future should have ended now
        delete _future;
        _future = nullptr;

        // then stop the function as well.
        delete _function;
        _function = nullptr;
      }

      /**
       * \brief Start the worker thread so we can monitor for events.
       */
      void Common::StartWorkerThread()
      {
        MYODDWEB_PROFILE_FUNCTION();

        // stop the old one... if any
        StopAndResetThread();

        // we must no longer stop
        _mustStop = false;

        // create the function
        _function = new Data::DataCallbackFunction(std::bind(&Common::DataCallbackFunction, this, std::placeholders::_1));

        // what we are looking for.
        // https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstchangenotificationa
        // https://docs.microsoft.com/en-gb/windows/desktop/api/WinBase/nf-winbase-readdirectorychangesw
        const auto notifyFilter = GetNotifyFilter();

        // create the data
        _data = new Data(_parent, notifyFilter, _parent.Recursive(), *_function, _bufferLength);

        // we can now looking for changes.
        _future = new std::future<void>(std::async( std::launch::async, &Common::Run, this));
      }

      /**
       * \brief check if we have to stop the current work.
       * \return bool if we have to stop or not.
       */
      bool Common::MustStop() const
      {
        return _mustStop;
      }

      /**
       * \brief Begin the actual work
       */
      void Common::Run()
      {
        // try and open the directory
        // if it is open already then nothing should happen here.
        if (!_data->Open())
        {
          // we could not access this
          _parent.AddEventError(EventError::Access);
          return;
        }

        // start reading.
        Read();

        // invalid handle wait
        auto invalidHandleWait = 0;

        auto concurentThreadsSupported = std::thread::hardware_concurrency();
        if (0 == concurentThreadsSupported)
        {
          concurentThreadsSupported = 1;
        }

        // the amount of time we want to put our thread to sleep
        // so we do not cause a tight loop to burn the CPU.
        const auto threadSleep = std::chrono::milliseconds(MYODDWEB_CPU_THREAD_SLEEP);

        auto count = 0;
        for (;;)
        {
          if (MustStop())
          {
            break;
          }

          if (!_data->IsValidHandle())
          {
            // wait a little bit longer.
            invalidHandleWait += MYODDWEB_MIN_THREAD_SLEEP;
            if (invalidHandleWait >= MYODDWEB_INVALID_HANDLE_SLEEP)
            {
              // reset the wait time.
              invalidHandleWait = 0;

              // try to re-open
              if (_data->TryReopen())
              {
                // try to read.
                Read();
              }
            }
          }
          else
          {
            // The handle is good, so we can reset the value
            invalidHandleWait = 0;
          }

          // sleep a bit, we must be alertable so we can pass/receive messages.
          ::SleepEx(MYODDWEB_MIN_THREAD_SLEEP, true);

          // we now need to slow the thread down a little more
          if (count % concurentThreadsSupported != 0)
          {
            std::this_thread::sleep_for(threadSleep);
            ++count;
          }
          else
          {
            std::this_thread::yield();
            count = 1;
          }
        }

        // if we are here... we can release the data
        _data->Close();
      }

      /**
       * \brief Start the read process
       */
      void Common::Read()
      {
        MYODDWEB_PROFILE_FUNCTION();

        if (!_data->IsValidHandle())
        {
          return;
        }

        // start the read now.
        if (!_data->Start())
        {
          // we could not create the monitoring
          // so we might as well get out now.
          _parent.AddEventError(EventError::Access);
          _data->Close();
        }
      }

      /***
       * \brief The async callback function for ReadDirectoryChangesW
       */
      void Common::DataCallbackFunction(unsigned char* pBufferBk)
      {
        MYODDWEB_PROFILE_FUNCTION();

        // Get the new read issued as fast as possible. The documentation
        // says that the original OVERLAPPED structure will not be used
        // again once the completion routine is called.
        Read();

        // If the buffer overflows, the entire contents of the buffer are discarded, 
        // the lpBytesReturned parameter contains zero, and the ReadDirectoryChangesW function 
        // fails with the error code ERROR_NOTIFY_ENUM_DIR.
        if (nullptr == pBufferBk)
        {
          return;
        }

        // we cloned the data and restarted the read
        // so we can now process the data
        // @todo this should be moved to a thread.
        ProcessNotificationFromBackup(pBufferBk);
      }

      /**
       * \brief this function is called _after_ we received a folder change request
       *        we own this buffer and we mus delete it at the end.
       * \param pBuffer
       */
      void Common::ProcessNotificationFromBackup(const unsigned char* pBuffer) const
      {
        MYODDWEB_PROFILE_FUNCTION();

        try
        {
          // overflow
          if (nullptr == pBuffer)
          {
            _parent.AddEventError(EventError::Overflow);
            return;
          }

          // rename filenames.
          std::wstring newFilename;
          std::wstring oldFilename;

          // get the file information
          auto pRecord = (FILE_NOTIFY_INFORMATION*)pBuffer;
          for (;;)
          {
            // get out now if needed
            if (MustStop())
            {
              break;
            }

            // get the filename
            const auto wFilename = std::wstring(pRecord->FileName, pRecord->FileNameLength / sizeof(wchar_t));
            switch (pRecord->Action)
            {
            case FILE_ACTION_ADDED:
              _parent.AddEvent(EventAction::Added, wFilename, IsFile(EventAction::Added, wFilename));
              break;

            case FILE_ACTION_REMOVED:
              _parent.AddEvent(EventAction::Removed, wFilename, IsFile(EventAction::Removed, wFilename));
              break;

            case FILE_ACTION_MODIFIED:
              _parent.AddEvent(EventAction::Touched, wFilename, IsFile(EventAction::Touched, wFilename));
              break;

            case FILE_ACTION_RENAMED_OLD_NAME:
              oldFilename = wFilename;
              if (!newFilename.empty())
              {
                // if we already have a new filename then we can add the rename event
                // and then clear both filenames so we do not add again
                _parent.AddRenameEvent(newFilename, oldFilename, IsFile(EventAction::Renamed, newFilename));
                newFilename = oldFilename = L"";
              }
              break;

            case FILE_ACTION_RENAMED_NEW_NAME:
              newFilename = wFilename;
              if (!oldFilename.empty())
              {
                // if we already have an old filename then we can add the rename event
                // and then clear both filenames so we do not add again
                _parent.AddRenameEvent(newFilename, oldFilename, IsFile(EventAction::Renamed, newFilename));
                newFilename = oldFilename = L"";
              }
              break;

            default:
              _parent.AddEvent(EventAction::Unknown, wFilename, IsFile(EventAction::Unknown, wFilename));
              break;
            }

            // more files?
            if (0 == pRecord->NextEntryOffset)
            {
              break;
            }
            pRecord = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&reinterpret_cast<unsigned char*>(pRecord)[pRecord->NextEntryOffset]);
          }

          // check for orphan renames...
          if (!oldFilename.empty())
          {
            _parent.AddEvent(EventAction::Removed, oldFilename, IsFile(EventAction::Removed, oldFilename));
          }
          if (!newFilename.empty())
          {
            _parent.AddEvent(EventAction::Added, newFilename, IsFile(EventAction::Added, newFilename));
          }
        }
        catch (...)
        {
          // regadless what happens
          // we have to free the memory.
          _parent.AddEventError(EventError::Memory);
        }

        // we are done with this buffer.
        delete[] pBuffer;
      }

      /**
       * \brief check if a given string is a file or a directory.
       * \param action the action we are looking at
       * \param path the file we are checking.
       * \return if the string given is a file or not.
       */
      bool Common::IsFile(const EventAction action, const std::wstring& path) const
      {
        try
        {
          const auto fullPath = Io::Combine(_parent.Path(), path);
          return Io::IsFile(fullPath);
        }
        catch (...)
        {
          return false;
        }
      }
    }
  }
}
