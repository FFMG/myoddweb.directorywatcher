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
#include <Windows.h>
#include <future>

namespace myoddweb
{
  namespace directorywatcher
  {
    class MonitorReadDirectoryChanges : public Monitor
    {
    public:
      MonitorReadDirectoryChanges(__int64 id, const Request& request);

    protected:
      MonitorReadDirectoryChanges(__int64 id, const Request& request, unsigned long bufferLength);

    public:
      virtual ~MonitorReadDirectoryChanges();

      bool Start() override;
      void Stop() override;

    protected:
      static void CALLBACK FileIoCompletionRoutine(
        DWORD dwErrorCode,							  // completion code
        DWORD dwNumberOfBytesTransfered,	// number of bytes transferred
        _OVERLAPPED* lpOverlapped         // I/O information buffer
      );

      static void RunThread(MonitorReadDirectoryChanges* obj);

    private:
      #pragma region
      void ResetBuffer();
      void Reset();
      void StopAndResetThread();
      #pragma endregion Reset/Stop functions

      bool OpenDirectory();
      bool IsOpen() const;
      void ProcessNotificationFromBackup(const unsigned char* pBuffer) const;
      unsigned char* Clone(unsigned long ulSize) const;

      void Read();
      void Run();

    private:
      HANDLE _hDirectory;
      unsigned char* _buffer;
      const unsigned long _bufferLength;

      /**
       * \brief the overlapped
       */
      OVERLAPPED	_overlapped{};

      #pragma region
      /**
       * \brief signal to stop the thread.
       */
      std::promise<void> _exitSignal;
      std::future<void> _futureObj;
      std::thread* _th;
      #pragma endregion Thread variables

      void StartWorkerThread();
    };
  }
}