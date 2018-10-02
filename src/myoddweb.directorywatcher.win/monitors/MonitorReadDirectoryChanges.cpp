#include "MonitorReadDirectoryChanges.h"
#include <string>
#include <process.h>

#define BUFFER_SIZE (unsigned long)16384

MonitorReadDirectoryChanges::MonitorReadDirectoryChanges(__int64 id, const std::wstring& path, bool recursive) :
  Monitor(id, path, recursive ),
  _hDirectory( 0 ),
  _th( nullptr ),
  _buffer( nullptr )
{
  memset(&_overlapped, 0, sizeof(OVERLAPPED));
}

MonitorReadDirectoryChanges::~MonitorReadDirectoryChanges()
{
  CloseDirectory();
}

void MonitorReadDirectoryChanges::CloseDirectory()
{
  if (IsOpen() )
  {
    CloseHandle(_hDirectory);
  }
  // the directory is closed.
  _hDirectory = nullptr;

  // stop the thread
  StopWorkerThread();

  // stop the bufdfer
  CompleteBuffer();
  memset(&_overlapped, 0, sizeof(OVERLAPPED));
}

void MonitorReadDirectoryChanges::CompleteBuffer()
{
  if (_buffer == nullptr)
  {
    return;
  }
  delete[] _buffer;
  _buffer = nullptr;
}

void MonitorReadDirectoryChanges::StopWorkerThread()
{
  if (_th == nullptr)
  {
    return;
  }

  // signal the stop
  _exitSignal.set_value();

  // wait a little
  _th->join();

  // cleanup
  delete _th;
  _th = nullptr;
}

bool MonitorReadDirectoryChanges::OpenDirectory()
{
  if (_hDirectory != nullptr)
  {
    return _hDirectory != INVALID_HANDLE_VALUE;
  }

  const auto shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  const auto fileAttr = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;

  _hDirectory = ::CreateFileW(
      Path().c_str(),					    // the path we are watching
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

void MonitorReadDirectoryChanges::Stop()
{
  CloseDirectory();
}

/**
 * @see https://docs.microsoft.com/en-gb/windows/desktop/api/winbase/nf-winbase-readdirectorychangesexw
 */
bool MonitorReadDirectoryChanges::Start()
{
  // close everything
  CloseDirectory();

  // set the variables we will be using here.
  _buffer = new char[BUFFER_SIZE];
  memset(_buffer, 0, sizeof(char)*BUFFER_SIZE);
  _overlapped.hEvent = this;

  // start the worker thread
  // in turn it will start the reading.
  StartWorkerThread();

  return true;
}

void MonitorReadDirectoryChanges::StartWorkerThread()
{
  // stop the old one... if any
  StopWorkerThread();

  // and start it again.
  _futureObj = _exitSignal.get_future();

  // we can now looking for changes.
  _th = new std::thread(&MonitorReadDirectoryChanges::BeginThread, this);
}

/**
 * We are now in the thread itself.
 */
void MonitorReadDirectoryChanges::BeginThread(MonitorReadDirectoryChanges* obj)
{
  // Run the thread.
  obj->Run();
}

/**
 * Begin the actual work
 */
void MonitorReadDirectoryChanges::Run()
{
  // try and open the directory
  if (!OpenDirectory())
  {
    return;
  }
  
  // start reading.
  Read();

  // now we keep on waiting.
  while (_futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
  {
    SleepEx(100, true );
  }
}

bool MonitorReadDirectoryChanges::IsOpen() const
{
  return _hDirectory != nullptr && _hDirectory != INVALID_HANDLE_VALUE;
}

void MonitorReadDirectoryChanges::Read()
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
    Recursive(),
    flags,
    nullptr,                  // bytes returned, (not used here as we are async)
    &_overlapped,             // buffer with our information
    &FileIoCompletionRoutine
  );
}

/***
 * The async callback function for ReadDirectoryChangesW
 */
void CALLBACK MonitorReadDirectoryChanges::FileIoCompletionRoutine(
  DWORD dwErrorCode,
  DWORD dwNumberOfBytesTransfered,
  LPOVERLAPPED lpOverlapped
)
{
  // get the object we are working with
  auto obj = (MonitorReadDirectoryChanges*)lpOverlapped->hEvent;

  if (dwErrorCode == ERROR_OPERATION_ABORTED)
  {
    return;
  }

  // If the buffer overflows, the entire contents of the buffer are discarded, 
  // the lpBytesReturned parameter contains zero, and the ReadDirectoryChangesW function 
  // fails with the error code ERROR_NOTIFY_ENUM_DIR.
  if (0 == dwNumberOfBytesTransfered)
  {
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
  obj->ProcessNotificationFromBackupPointer(pBufferBk);
}

void* MonitorReadDirectoryChanges::Clone(unsigned long ulSize)
{
  // we have an overfolow
  if (ulSize > BUFFER_SIZE)
  {
    return nullptr;
  }

  // create the clone
  const auto pBuffer = new char[ulSize];

  // copy it.
  memcpy(pBuffer, _buffer, ulSize);

  return pBuffer;
}

void MonitorReadDirectoryChanges::ProcessNotificationFromBackupPointer(const void* pBuffer )
{
  try
  {
    // overflow
    if (nullptr == pBuffer)
    {
      return;
    }

    // get the file information
    auto* pRecord = (FILE_NOTIFY_INFORMATION*)pBuffer;;
    for (;;)
    {
      // get the filename
      auto wFilename = std::wstring(pRecord->FileName, pRecord->FileNameLength / sizeof(wchar_t));
      wprintf(L"%s\\%s\n", Path().c_str(), wFilename.c_str());

      // more files?
      if (0 == pRecord->NextEntryOffset)
      {
        break;
      }
      pRecord = (FILE_NOTIFY_INFORMATION *)(&((unsigned char*)pRecord)[pRecord->NextEntryOffset]);
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