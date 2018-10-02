#pragma once
#include "Monitor.h"
#include <string>
#include <Windows.h>
#include <future>

class MonitorReadDirectoryChangesEx : public Monitor
{
public:
  MonitorReadDirectoryChangesEx(__int64 id);
  virtual ~MonitorReadDirectoryChangesEx();

  bool Poll(const std::wstring& path, bool recursive);
  void Stop();

protected:
  static void CALLBACK FileIoCompletionRoutine(
    DWORD dwErrorCode,							  // completion code
    DWORD dwNumberOfBytesTransfered,	// number of bytes transferred
    LPOVERLAPPED lpOverlapped         // I/O information buffer
  );

  static unsigned int WINAPI BeginThread(LPVOID arg);

private:
  void CompleteThread();
  void CompleteBuffer();

  bool OpenDirectory();
  void CloseDirectory();
  void WaitForRead();
  static void CALLBACK WaitForRead(unsigned long pointer );
  bool IsOpen() const;
  void ProcessNotificationFromBackupPointer(const void* pBufferBk);
  void* Clone(unsigned long ulSize);
  

private:
  HANDLE _hDirectory;
  std::wstring _path;
  void* _buffer;

  OVERLAPPED	_overlapped;
  bool _recursive;

  // Create a std::promise object
  std::promise<void> _exitSignal;
  std::future<void> _futureObj;
  HANDLE _th;
};

