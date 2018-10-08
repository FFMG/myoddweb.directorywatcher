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
#include <windows.h>
#include "MonitorReadDirectoryChangesCommon.h"
#include "..\utils\Io.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Create the Monitor that uses ReadDirectoryChanges
     */
    MonitorReadDirectoryChangesCommon::MonitorReadDirectoryChangesCommon(
      Monitor& parent,
      const unsigned long bufferLength) :
      _hDirectory(nullptr),
      _buffer(nullptr),
      _bufferLength(bufferLength),
      _parent( parent ),
      _th(nullptr)
    {
      memset(&_overlapped, 0, sizeof(OVERLAPPED));
    }

    MonitorReadDirectoryChangesCommon::~MonitorReadDirectoryChangesCommon()
    {
      Reset();
    }

    /**
     * \brief https://docs.microsoft.com/en-gb/windows/desktop/api/winbase/nf-winbase-readdirectorychangesexw
     * \return success or not.
     */
    bool MonitorReadDirectoryChangesCommon::Start()
    {
      // close everything
      Reset();

      // create the buffer.
      _buffer = new unsigned char[_bufferLength];
      memset(_buffer, 0, sizeof(unsigned char)*_bufferLength);
      _overlapped.hEvent = this;

      // start the worker thread
      // in turn it will start the reading.
      StartWorkerThread();

      return true;
    }

    /**
     * \brief Stop monitoring
     */
    void MonitorReadDirectoryChangesCommon::Stop()
    {
      Reset();
    }

    /**
     * \brief Close all the handles, delete pointers and reset all the values.
     */
    void MonitorReadDirectoryChangesCommon::Reset()
    {
      if (IsOpen())
      {
        CloseHandle(_hDirectory);
      }
      // the directory is closed.
      _hDirectory = nullptr;

      // stop the thread
      StopAndResetThread();

      // stop the bufdfer
      ResetBuffer();

      // reset the overleapped.
      memset(&_overlapped, 0, sizeof(OVERLAPPED));
    }

    /**
     * \brief delete the buffer, (if used), and reset the value.
     */
    void MonitorReadDirectoryChangesCommon::ResetBuffer()
    {
      if (_buffer == nullptr)
      {
        return;
      }
      delete[] _buffer;
      _buffer = nullptr;
    }

    /**
     * \brief Stop the worker thread, wait for it to complete and then delete it.
     */
    void MonitorReadDirectoryChangesCommon::StopAndResetThread()
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
     * \brief Open the directory we want to watch
     * \return if there was a problem opening the file.
     */
    bool MonitorReadDirectoryChangesCommon::OpenDirectory()
    {
      // check if this was done alread
      // we cannot use IsOpen() as INVALID_HANDLE_VALUE would cause a return false.
      if (_hDirectory != nullptr)
      {
        return _hDirectory != INVALID_HANDLE_VALUE;
      }

      // how we want to open this directory.
      const auto shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
      const auto fileAttr = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;

      _hDirectory = ::CreateFileW(
        _parent.Path().c_str(),					    // the path we are watching
        FILE_LIST_DIRECTORY,        // required for ReadDirectoryChangesW( ... )
        shareMode,
        nullptr,                    // security descriptor
        OPEN_EXISTING,              // how to create
        fileAttr,
        nullptr                     // file with attributes to copy
      );

      // check if it all worked.
      if (IsOpen())
      {
        return true;
      }

      // we could not access this
      _parent.AddEventError(EventAction::ErrorAccess);

      return false;
    }

    /**
     * \brief Start the worker thread so we can monitor for events.
     */
    void MonitorReadDirectoryChangesCommon::StartWorkerThread()
    {
      // stop the old one... if any
      StopAndResetThread();

      // we can now reset our future
      // so we can cancel/stop the thread/
      _futureObj = _exitSignal.get_future();

      // we can now looking for changes.
      _th = new std::thread(&MonitorReadDirectoryChangesCommon::RunThread, this);
    }

    /**
     * \brief the worker thread that runs the code itself.
     * \param obj pointer to this instance of the class.
     */
    void MonitorReadDirectoryChangesCommon::RunThread(MonitorReadDirectoryChangesCommon* obj)
    {
      // Run the thread.
      obj->Run();
    }

    /**
     * \brief Begin the actual work
     */
    void MonitorReadDirectoryChangesCommon::Run()
    {
      // try and open the directory
      // if it is open already then nothing should happen here.
      if (!OpenDirectory())
      {
        _parent.AddEventError(EventAction::ErrorAccess);
        return;
      }

      // start reading.
      Read();

      // now we keep on waiting.
      while (_futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
      {
        SleepEx(100, true);
      }
    }

    /**
     * \brief Check if the file is open properly
     * \return if the file has been open already.
     */
    bool MonitorReadDirectoryChangesCommon::IsOpen() const
    {
      return _hDirectory != nullptr && _hDirectory != INVALID_HANDLE_VALUE;
    }

    /**
     * \brief Start the read process
     */
    void MonitorReadDirectoryChangesCommon::Read()
    {
      if (!IsOpen())
      {
        return;
      }

      // what we are looking for.
      // https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstchangenotificationa
      // https://docs.microsoft.com/en-gb/windows/desktop/api/WinBase/nf-winbase-readdirectorychangesw
      const auto notifyFilter = GetNotifyFilter();

      // This call needs to be reissued after every APC.
      if (!::ReadDirectoryChangesW(
        _hDirectory,
        _buffer,
        _bufferLength,
        _parent.Recursive(),
        notifyFilter,
        nullptr,                  // bytes returned, (not used here as we are async)
        &_overlapped,             // buffer with our information
        &FileIoCompletionRoutine
      ))
      {
        _parent.AddEventError(EventAction::ErrorCannotStart);
      }
    }

    /***
     * \brief The async callback function for ReadDirectoryChangesW
     */
    void CALLBACK MonitorReadDirectoryChangesCommon::FileIoCompletionRoutine(
      const unsigned long dwErrorCode,
      const unsigned long dwNumberOfBytesTransfered,
      _OVERLAPPED* lpOverlapped
    )
    {
      // get the object we are working with
      auto obj = static_cast<MonitorReadDirectoryChangesCommon*>(lpOverlapped->hEvent);

      if (dwErrorCode == ERROR_OPERATION_ABORTED)
      {
        obj->_parent.AddEventError(EventAction::ErrorAborted);
        return;
      }

      // If the buffer overflows, the entire contents of the buffer are discarded, 
      // the lpBytesReturned parameter contains zero, and the ReadDirectoryChangesW function 
      // fails with the error code ERROR_NOTIFY_ENUM_DIR.
      if (0 == dwNumberOfBytesTransfered)
      {
        obj->_parent.AddEventError(EventAction::ErrorOverflow);

        // noting to read, just restart
        obj->Read();
        return;
      }

      // Can't use sizeof(FILE_NOTIFY_INFORMATION) because
      // the structure is padded to 16 bytes.
      _ASSERTE(dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR));

      // clone the data so we can start reading right away.
      const auto pBufferBk = obj->Clone(dwNumberOfBytesTransfered);

      // Get the new read issued as fast as possible. The documentation
      // says that the original OVERLAPPED structure will not be used
      // again once the completion routine is called.
      obj->Read();

      // we cloned the data and restarted the read
      // so we can now process the data
      // @todo this should be moved to a thread.
      obj->ProcessNotificationFromBackup(pBufferBk);
    }

    /**
     * \brief Clone the buffer into a temp buffer, we will pass ownership of the new buffer to the caller.
     * \param ulSize the number of bytes we want to copy over.
     * \return unsigned char* the newly created buffer.
     */
    unsigned char* MonitorReadDirectoryChangesCommon::Clone(const unsigned long ulSize) const
    {
      // if the size if more than we can offer we need to prevent an overflow.
      if (ulSize > _bufferLength)
      {
        return nullptr;
      }

      try
      {
        // create the clone
        const auto pBuffer = new unsigned char[ulSize];

        // copy it.
        memcpy(pBuffer, _buffer, ulSize);

        // return it.
        return pBuffer;
      }
      catch (...)
      {
        _parent.AddEventError(EventAction::ErrorMemory);
        return nullptr;
      }
    }

    /**
     * \brief this function is called _after_ we received a folder change request
     *        we own this buffer and we mus delete it at the end.
     * \param pBuffer
     */
    void MonitorReadDirectoryChangesCommon::ProcessNotificationFromBackup(const unsigned char* pBuffer) const
    {
      try
      {
        // overflow
        if (nullptr == pBuffer)
        {
          _parent.AddEventError(EventAction::ErrorOverflow);
          return;
        }

        // rename filenames.
        std::wstring newFilename;
        std::wstring oldFilename;

        // get the file information
        auto pRecord = (FILE_NOTIFY_INFORMATION*)pBuffer;;
        for (;;)
        {
          // get the filename
          const auto wFilename = std::wstring(pRecord->FileName, pRecord->FileNameLength / sizeof(wchar_t));
          switch (pRecord->Action)
          {
          case FILE_ACTION_ADDED:
            _parent.AddEvent(EventAction::Added, wFilename, IsFile(EventAction::Added,wFilename));
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
        _parent.AddEventError(EventAction::ErrorMemory);
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
    bool MonitorReadDirectoryChangesCommon::IsFile(const EventAction action, const std::wstring& path) const
    {
      try
      {
        const auto fullPath = Io::Combine(_parent.Path(), path);
        return ((GetFileAttributesW(fullPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY) == 0);
      }
      catch (...)
      {
        return false;
      }
    }
  }
}
