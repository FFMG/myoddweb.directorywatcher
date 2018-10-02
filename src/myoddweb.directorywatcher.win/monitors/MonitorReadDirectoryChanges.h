#pragma once
#include "Monitor.h"
#include <string>
#include <Windows.h>
#include <future>

class MonitorReadDirectoryChanges : public Monitor
{
public:
  MonitorReadDirectoryChanges(__int64 id, const std::wstring& path, bool recursive);
  virtual ~MonitorReadDirectoryChanges();

  bool Start();
  void Stop();

protected:
  static void CALLBACK FileIoCompletionRoutine(
    DWORD dwErrorCode,							  // completion code
    DWORD dwNumberOfBytesTransfered,	// number of bytes transferred
    LPOVERLAPPED lpOverlapped         // I/O information buffer
  );

  static void BeginThread(MonitorReadDirectoryChanges* obj);

private:
  void CompleteBuffer();

  bool OpenDirectory();
  void CloseDirectory();
  bool IsOpen() const;
  void ProcessNotificationFromBackupPointer(const void* pBufferBk);
  void* Clone(unsigned long ulSize);

  void Read();
  void Run();

private:
  HANDLE _hDirectory;
  void* _buffer;

  OVERLAPPED	_overlapped;

  // Create a std::promise object
  std::promise<void> _exitSignal;
  std::future<void> _futureObj;
  std::thread* _th;

  void StopWorkerThread();
  void StartWorkerThread();
};

