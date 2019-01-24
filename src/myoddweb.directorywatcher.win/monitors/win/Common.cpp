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
#include <Windows.h>
#include "Common.h"
#include "../../utils/Io.h"
#include "../../utils/EventError.h"
#include "../../utils/MonitorsManager.h"
#include "../Base.h"

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
        _data(nullptr),
        _parent(parent),
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
        // close everything
        Stop();

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
        if (_th == nullptr)
        {
          return;
        }

        // signal the stop
        _exitSignal.set_value();

        // wait a little
        if (_th->joinable())
        {
          _th->join();
        }

        // cleanup
        delete _th;
        _th = nullptr;
      }

      /**
       * \brief Start the worker thread so we can monitor for events.
       */
      void Common::StartWorkerThread()
      {
        // stop the old one... if any
        StopAndResetThread();

        // start the data
        _data = new Data(_parent, _bufferLength);

        // we can now reset our future
        // so we can cancel/stop the thread/
        _futureObj = _exitSignal.get_future();

        // we can now looking for changes.
        _th = new std::thread(&Common::RunThread, this);
      }

      /**
       * \brief the worker thread that runs the code itself.
       * \param obj pointer to this instance of the class.
       */
      void Common::RunThread(Common* obj)
      {
        // Run the thread.
        obj->Run();
      }

      /**
       * \brief Begin the actual work
       */
      void Common::Run()
      {
        // try and open the directory
        // if it is open already then nothing should happen here.
        if (!_data->Open() )
        {
          // we could not access this
          _parent.AddEventError(EventError::Access);
          return;
        }

        // start reading.
        Read();

        // now we keep on waiting.
        while (_futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
        {
          // wait a little.
          SleepEx(MYODDWEB_SLEEP_BETWEEN_READS, true);
        }

        // if we are here... we can release the data
        _data->Close();
      }

      /**
       * \brief Start the read process
       */
      void Common::Read()
      {
        if (!_data->IsValidHandle() )
        {
          return;
        }

        // what we are looking for.
        // https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstchangenotificationa
        // https://docs.microsoft.com/en-gb/windows/desktop/api/WinBase/nf-winbase-readdirectorychangesw
        const auto notifyFilter = GetNotifyFilter();

        if( !_data->Start( this, notifyFilter, _parent.Recursive(), &FileIoCompletionRoutine))
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
      void CALLBACK Common::FileIoCompletionRoutine(
        const unsigned long mumberOfBytesTransfered,
        void* object,
        Data& data
      )
      {
        // get the object we are working with
        auto obj = static_cast<Common*>(object);

        // If the buffer overflows, the entire contents of the buffer are discarded, 
        // the lpBytesReturned parameter contains zero, and the ReadDirectoryChangesW function 
        // fails with the error code ERROR_NOTIFY_ENUM_DIR.
        if (0 == mumberOfBytesTransfered)
        {
          // noting to read, just restart
          if (obj != nullptr)
          {
            obj->Read();
          }
          return;
        }

        // Can't use sizeof(FILE_NOTIFY_INFORMATION) because
        // the structure is padded to 16 bytes.
        _ASSERTE(mumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR));

        // clone the data so we can start reading right away.
        unsigned char* pBufferBk = nullptr;
        try
        {
          pBufferBk = data.Clone(mumberOfBytesTransfered);
        }
        catch (...)
        {
          if (obj != nullptr)
          {
            obj->_parent.AddEventError(EventError::Memory);
          }
        }

        // Get the new read issued as fast as possible. The documentation
        // says that the original OVERLAPPED structure will not be used
        // again once the completion routine is called.
        if (obj != nullptr)
        {
          obj->Read();

          // we cloned the data and restarted the read
          // so we can now process the data
          // @todo this should be moved to a thread.
          obj->ProcessNotificationFromBackup(pBufferBk);
        }
      }

      /**
       * \brief this function is called _after_ we received a folder change request
       *        we own this buffer and we mus delete it at the end.
       * \param pBuffer
       */
      void Common::ProcessNotificationFromBackup(const unsigned char* pBuffer) const
      {
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
            pRecord = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(&reinterpret_cast<unsigned char*>(pRecord)[pRecord->NextEntryOffset]);
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
