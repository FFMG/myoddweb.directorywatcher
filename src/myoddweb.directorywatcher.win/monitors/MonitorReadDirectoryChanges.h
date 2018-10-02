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

