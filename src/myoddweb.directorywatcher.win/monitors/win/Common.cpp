// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "Common.h"
#include "../../utils/Io.h"
#include "../../utils/EventError.h"
#include "../../utils/Instrumentor.h"

namespace myoddweb ::directorywatcher :: win
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
    // clear the data.
    delete _data;
    _data = nullptr;
  }

  bool Common::Start()
  {
    return CreateAndStartData();
  }

  bool Common::CreateAndStartData()
  {
    // what we are looking for.
    // https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstchangenotificationa
    // https://docs.microsoft.com/en-gb/windows/desktop/api/WinBase/nf-winbase-readdirectorychangesw
    const auto notifyFilter = GetNotifyFilter();

    // create the data
    _data = new Data(
      _parent.Id(),
      _parent.Path(),
      notifyFilter, 
      _parent.Recursive(), 
      _bufferLength,
      _parent.WorkerPool() 
    );

    // then start monitoring
    return _data->Start();
  }

  void Common::Update() const
  {
    // check if we have stoped
    if( nullptr == _data)
    {
      return;
    }

    // get the data and then process it
    const auto rawData = _data->Get();
    for( const auto& raw : rawData )
    {
      ProcessNotification(raw);
      delete[] raw;
    }

    // ensure that the data is still valid
    _data->CheckStillValid();
  }

  /**
   * \brief complete all the data collection
   */
  void Common::Stop()
  {
    if (nullptr != _data)
    {
      // if we are here... we can release the data
      _data->Stop();
    }
  }

  /**
   * \brief this function is called _after_ we received a folder change request
   *        we own this buffer and we mus delete it at the end.
   * \param pBuffer
   */
  void Common::ProcessNotification(const unsigned char* pBuffer) const
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
