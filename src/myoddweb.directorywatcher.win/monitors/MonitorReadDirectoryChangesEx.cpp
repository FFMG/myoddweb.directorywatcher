#include "MonitorReadDirectoryChangesEx.h"
#include <string>
#include <process.h>

#define BUFFER_SIZE (unsigned long)16384

MonitorReadDirectoryChangesEx::MonitorReadDirectoryChangesEx(__int64 id) :
  Monitor(id),
  _hDirectory( 0 ),
  _path( L"" ),
  _recursive( false ),
  _th( nullptr ),
  _buffer( nullptr )
{
  memset(&_overlapped, 0, sizeof(OVERLAPPED));
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
  // the directory is closed.
  _hDirectory = nullptr;

  // stop the thread
  CompleteThread();

  // stop the bufdfer
  CompleteBuffer();

  // reset other values.
  _path = L"";
  _recursive = false;
  memset(&_overlapped, 0, sizeof(OVERLAPPED));
}

void MonitorReadDirectoryChangesEx::CompleteBuffer()
{
  if (_buffer == nullptr)
  {
    return;
  }
  delete[] _buffer;
  _buffer = nullptr;
}

void MonitorReadDirectoryChangesEx::CompleteThread()
{
  if (_th == nullptr)
  {
    return;
  }

  _exitSignal.set_value();

  // set the value in promise
  _th = nullptr;
}

bool MonitorReadDirectoryChangesEx::OpenDirectory()
{
  if (_hDirectory != nullptr)
  {
    return _hDirectory != INVALID_HANDLE_VALUE;
  }

  const auto shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  const auto fileAttr = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;

  _hDirectory = ::CreateFileW(
      _path.c_str(),					    // the path we are watching
      FILE_LIST_DIRECTORY,        // required for ReadDirectoryChangesW( ... )
      shareMode,
      nullptr,                    // security descriptor
      OPEN_EXISTING,              // how to create
      fileAttr,
      nullptr                     // file with attributes to copy
    );

  // check if it all worked.
  return IsOpen();
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

  _futureObj = _exitSignal.get_future();

  _th = (HANDLE)_beginthreadex(nullptr,
    0,
    BeginThread,
    this,
    0,
    nullptr
  );

  // set the variables we will be using here.
  _buffer = new char[BUFFER_SIZE];
  memset(_buffer, 0, sizeof(char)*BUFFER_SIZE);
  _overlapped.hEvent = this;
  _path = path;
  _recursive = recursive;

  // try and open the directory
  if (!OpenDirectory())
  {
    return false;
  }

  // we can now looking for changes.
  QueueUserAPC(WaitForRead, _th, (ULONG_PTR)this );

  return true;
}

unsigned int WINAPI MonitorReadDirectoryChangesEx::BeginThread(LPVOID arg)
{
  while (((MonitorReadDirectoryChangesEx*)arg)->_futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return 0;
}

bool MonitorReadDirectoryChangesEx::IsOpen() const
{
  return _hDirectory != nullptr && _hDirectory != INVALID_HANDLE_VALUE;
}

void CALLBACK MonitorReadDirectoryChangesEx::WaitForRead(unsigned long pointer)
{
  ((MonitorReadDirectoryChangesEx*)pointer)->WaitForRead();
}

void MonitorReadDirectoryChangesEx::WaitForRead()
{
  if (!IsOpen())
  {
    return;
  }

  // what we are looking for.
  const auto flags = FILE_NOTIFY_CHANGE_FILE_NAME |
                     FILE_NOTIFY_CHANGE_DIR_NAME |  
                     FILE_NOTIFY_CHANGE_ATTRIBUTES |  
                     FILE_NOTIFY_CHANGE_SIZE |  
                     FILE_NOTIFY_CHANGE_LAST_WRITE |  
                     FILE_NOTIFY_CHANGE_LAST_ACCESS |  
                     FILE_NOTIFY_CHANGE_CREATION |  
                     FILE_NOTIFY_CHANGE_SECURITY;

  // This call needs to be reissued after every APC.
  auto result = ::ReadDirectoryChangesW(
    _hDirectory,
    _buffer,
    BUFFER_SIZE,
    _recursive,
    flags,
    nullptr,                  // bytes returned, (not used here as we are async)
    &_overlapped,             // buffer with our information
    &FileIoCompletionRoutine
  );
}

/***
 * The async callback function for ReadDirectoryChangesW
 */
void CALLBACK MonitorReadDirectoryChangesEx::FileIoCompletionRoutine(
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

  // If the buffer overflows, the entire contents of the buffer are discarded, 
  // the lpBytesReturned parameter contains zero, and the ReadDirectoryChangesW function 
  // fails with the error code ERROR_NOTIFY_ENUM_DIR.
  if (!dwNumberOfBytesTransfered)
  {
    // just restart
    pThis->WaitForRead();
    return;
  }

  const auto pBufferBk = pThis->Clone(dwNumberOfBytesTransfered);

  // Get the new read issued as fast as possible. The documentation
  // says that the original OVERLAPPED structure will not be used
  // again once the completion routine is called.
  pThis->WaitForRead();

  // we clones the data and restarted the read
  // so we can now process the data
  pThis->ProcessNotificationFromBackupPointer(pBufferBk);
}

void* MonitorReadDirectoryChangesEx::Clone(unsigned long ulSize)
{
  // create the clone
  const auto pBuffer = new char[ulSize];

  // copy it.
  memcpy(pBuffer, _buffer, ulSize);

  return pBuffer;
}

void MonitorReadDirectoryChangesEx::ProcessNotificationFromBackupPointer(const void* pBuffer )
{
  try
  {
    for (;;)
    {
      // get the file information
      auto* pRecord = (FILE_NOTIFY_INFORMATION*)pBuffer;;

      // get the filename
      auto wstrFilename = std::wstring(pRecord->FileName, pRecord->FileNameLength / sizeof(wchar_t));

      // more files?
      if (0 == pRecord->NextEntryOffset)
      {
        break;
      }
      pRecord = (FILE_NOTIFY_INFORMATION *)(&((unsigned char*)pBuffer)[pRecord->NextEntryOffset]);
    }
  }
  catch( ... )
  {
    // regadless what happens
    // we have to free the memory.
  }

  // we are done with this buffer.
  delete[] pBuffer;
}