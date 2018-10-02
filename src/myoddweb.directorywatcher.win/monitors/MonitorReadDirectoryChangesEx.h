#pragma once
#include "Monitor.h"
#include <string>
#include <vector>
#include <Windows.h>
#include <future>

class MonitorReadDirectoryChangesEx : public Monitor
{
public:
  MonitorReadDirectoryChangesEx(__int64 id);
  virtual ~MonitorReadDirectoryChangesEx();

public:
  bool Poll(const std::wstring& path, bool recursive);
  void Stop();

private:
  static void CALLBACK Callback(
    DWORD dwErrorCode,							  // completion code
    DWORD dwNumberOfBytesTransfered,	// number of bytes transferred
    LPOVERLAPPED lpOverlapped);				// I/O information buffer

private:
  bool OpenDirectory();
  void CloseDirectory();
  void WaitForRead();
  bool IsOpen() const;
  void ProcessNotification();
  void Clone(DWORD dwSize);
  static void BeginThread(MonitorReadDirectoryChangesEx* obj);

private:
  HANDLE _hDirectory;
  std::wstring _path;
  std::vector<byte> _buffer;
  std::vector<byte> _backupBuffer;

  OVERLAPPED	_overlapped;
  DWORD _flags;
  bool _recursive;

  // Create a std::promise object
  std::promise<void> _exitSignal;
  std::future<void> _futureObj;
  std::thread* _th;
};

