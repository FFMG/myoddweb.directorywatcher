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

  bool Common::start()
  {
    return create_and_start_data();
  }

  bool Common::create_and_start_data()
  {
    // what we are looking for.
    // https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstchangenotificationa
    // https://docs.microsoft.com/en-gb/windows/desktop/api/WinBase/nf-winbase-readdirectorychangesw
    const auto notifyFilter = get_notify_filter();

    // create the data
    _data = new Data(
      _parent.id(),
      _parent.path(),
      notifyFilter,
      _parent.recursive(),
      _bufferLength,
      _parent.worker_pool()
    );

    // then start monitoring
    return _data->start();
  }

  void Common::update() const
  {
    // check if we have stoped
    if( nullptr == _data)
    {
      return;
    }

    // get the data and then process it
    const auto rawData = _data->get();
    for( const auto& raw : rawData )
    {
      process_notification(raw);
      delete[] raw;
    }

    // ensure that the data is still valid
    _data->check_still_valid();
  }

  /**
   * \brief complete all the data collection
   */
  void Common::stop()
  {
    if (nullptr != _data)
    {
      // if we are here... we can release the data
      _data->stop();
    }
  }

  /**
   * \brief this function is called _after_ we received a folder change request
   *        we own this buffer and we mus delete it at the end.
   * \param pBuffer
   */
  void Common::process_notification(const unsigned char* pBuffer) const
  {
    MYODDWEB_PROFILE_FUNCTION();

    try
    {
      // overflow
      if (nullptr == pBuffer)
      {
        _parent.add_event_error(EventError::Overflow);
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
          _parent.add_event(EventAction::Added, wFilename, is_file(EventAction::Added, wFilename));
          break;

        case FILE_ACTION_REMOVED:
          _parent.add_event(EventAction::Removed, wFilename, is_file(EventAction::Removed, wFilename));
          break;

        case FILE_ACTION_MODIFIED:
          _parent.add_event(EventAction::Touched, wFilename, is_file(EventAction::Touched, wFilename));
          break;

        case FILE_ACTION_RENAMED_OLD_NAME:
          oldFilename = wFilename;
          if (!newFilename.empty())
          {
            // if we already have a new filename then we can add the rename event
            // and then clear both filenames so we do not add again
            _parent.add_rename_event(newFilename, oldFilename, is_file(EventAction::Renamed, newFilename));
            newFilename = oldFilename = L"";
          }
          break;

        case FILE_ACTION_RENAMED_NEW_NAME:
          newFilename = wFilename;
          if (!oldFilename.empty())
          {
            // if we already have an old filename then we can add the rename event
            // and then clear both filenames so we do not add again
            _parent.add_rename_event(newFilename, oldFilename, is_file(EventAction::Renamed, newFilename));
            newFilename = oldFilename = L"";
          }
          break;

        default:
          _parent.add_event(EventAction::Unknown, wFilename, is_file(EventAction::Unknown, wFilename));
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
        _parent.add_event(EventAction::Removed, oldFilename, is_file(EventAction::Removed, oldFilename));
      }
      if (!newFilename.empty())
      {
        _parent.add_event(EventAction::Added, newFilename, is_file(EventAction::Added, newFilename));
      }
    }
    catch (...)
    {
      // regadless what happens
      // we have to free the memory.
      _parent.add_event_error(EventError::Memory);
    }
  }

  /**
   * \brief check if a given string is a file or a directory.
   * \param action the action we are looking at
   * \param path the file we are checking.
   * \return if the string given is a file or not.
   */
  bool Common::is_file(const EventAction action, const std::wstring& path) const
  {
    try
    {
      const auto fullPath = Io::combine(_parent.path(), path);
      return Io::is_file(fullPath);
    }
    catch (...)
    {
      return false;
    }
  }
}
