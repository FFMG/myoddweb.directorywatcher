#include "MonitorReadDirectoryChangesEx.h"
#include <string>
#include <Windows.h>

#define BUFFER_SIZE (DWORD)16384
#define DEFAULT_FLAGS  FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION

MonitorReadDirectoryChangesEx::MonitorReadDirectoryChangesEx(__int64 id) :
  Monitor(id),
  _flags(DEFAULT_FLAGS ),
  _hDirectory( 0 ),
  _path( L"" ),
  _recursive( false ),
  _th( nullptr )
{
  memset(&_overlapped, 0, sizeof(OVERLAPPED));

  _buffer.resize(BUFFER_SIZE);
  _backupBuffer.resize(BUFFER_SIZE);
}

MonitorReadDirectoryChangesEx::~MonitorReadDirectoryChangesEx()
{
  CloseDirectory();
}

void MonitorReadDirectoryChangesEx::CloseDirectory()
{
  if (IsOpen() )
  {
    CloseHandle(_hDirectory);
  }

  if (_th != nullptr)
  {
    // set the value in promise
    _exitSignal.set_value();
    _th->join();
    delete _th;
  }

  _th = nullptr;
  _hDirectory = nullptr;
  _path = L"";
  _recursive = false;
  memset(&_overlapped, 0, sizeof(OVERLAPPED));
}

bool MonitorReadDirectoryChangesEx::OpenDirectory()
{
  if (_hDirectory != nullptr)
  {
    return _hDirectory != INVALID_HANDLE_VALUE;
  }

  _hDirectory = ::CreateFileW(
    _path.c_str(),					      // pointer to the file name
      FILE_LIST_DIRECTORY,        // access (read/write) mode
      FILE_SHARE_READ					    // share mode
      | FILE_SHARE_WRITE
      | FILE_SHARE_DELETE,
      NULL,                       // security descriptor
      OPEN_EXISTING,              // how to create
      FILE_FLAG_BACKUP_SEMANTICS	// file attributes
      | FILE_FLAG_OVERLAPPED,
      NULL);                      // file with attributes to copy
}

void MonitorReadDirectoryChangesEx::Stop()
{
  CloseDirectory();
}

/**
 * @see https://docs.microsoft.com/en-gb/windows/desktop/api/winbase/nf-winbase-readdirectorychangesexw
 */
bool MonitorReadDirectoryChangesEx::Poll( const std::wstring& path, bool recursive)
{
  // close everything
  CloseDirectory();

  // set the path
  _futureObj = _exitSignal.get_future();
  _overlapped.hEvent = this;
  _path = path;
  _recursive = recursive;

  // try and open the directory
  if (!OpenDirectory())
  {
    return false;
  }

  // we can now looking for changes.
  _th = new std::thread(&MonitorReadDirectoryChangesEx::BeginThread, this);
}

void MonitorReadDirectoryChangesEx::BeginThread(MonitorReadDirectoryChangesEx* obj)
{
  obj->WaitForRead();

  while (obj->_futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

bool MonitorReadDirectoryChangesEx::IsOpen() const
{
  return _hDirectory != nullptr && _hDirectory != INVALID_HANDLE_VALUE;
}

void MonitorReadDirectoryChangesEx::WaitForRead()
{
  if (!IsOpen())
  {
    return;
  }
  DWORD dwBytes = 0;

  // This call needs to be reissued after every APC.
  BOOL success = ::ReadDirectoryChangesW(
    _hDirectory,
    &_buffer[0],
    _buffer.size(),   // length of buffer
    _recursive,
    _flags,
    &dwBytes,         // bytes returned
    &_overlapped,     // overlapped buffer
    &Callback);       // completion routine
}

//static
void CALLBACK MonitorReadDirectoryChangesEx::Callback(
  DWORD dwErrorCode,
  DWORD dwNumberOfBytesTransfered,
  LPOVERLAPPED lpOverlapped
)
{
  auto pThis = (MonitorReadDirectoryChangesEx*)lpOverlapped->hEvent;

  if (dwErrorCode == ERROR_OPERATION_ABORTED)
  {
    delete pThis;
    return;
  }

  // Can't use sizeof(FILE_NOTIFY_INFORMATION) because
  // the structure is padded to 16 bytes.
  _ASSERTE(dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR));

  // do we have any data to process?
  if (!dwNumberOfBytesTransfered)
  {
    // just restart
    pThis->WaitForRead();
    return;
  }

  pThis->Clone(dwNumberOfBytesTransfered);

  // Get the new read issued as fast as possible. The documentation
  // says that the original OVERLAPPED structure will not be used
  // again once the completion routine is called.
  pThis->WaitForRead();

  pThis->ProcessNotification();
}

void MonitorReadDirectoryChangesEx::Clone(DWORD dwSize)
{
  // We could just swap back and forth between the two
  // buffers, but this code is easier to understand and debug.
  memcpy(&_backupBuffer[0], &_buffer[0], dwSize);
}

void MonitorReadDirectoryChangesEx::ProcessNotification()
{
  auto pBase = _backupBuffer.data();

  for (;;)
  {
    FILE_NOTIFY_INFORMATION& fni = (FILE_NOTIFY_INFORMATION&)*pBase;

    auto wstrFilename = std::wstring(fni.FileName, fni.FileNameLength / sizeof(wchar_t));

    if (!fni.NextEntryOffset)
    {
      break;
    }
    pBase += fni.NextEntryOffset;
  };
}