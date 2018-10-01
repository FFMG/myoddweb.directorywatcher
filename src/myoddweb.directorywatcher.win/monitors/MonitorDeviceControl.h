#pragma once
#include <Windows.h>
#include "Monitor.h"

#define BUFFER_SIZE (1024 * 1024)

class MonitorDeviceControl : public Monitor
{
public:
  explicit MonitorDeviceControl(__int64 id);
  virtual ~MonitorDeviceControl() = default;

private:
  HANDLE _drive;
  USN _maxusn;
  MFT_ENUM_DATA_V0 _mft_enum_data;
  DWORD _bytecount;
  void * _buffer;
  USN_RECORD * _record;
  USN_RECORD * _recordend;
  USN_JOURNAL_DATA * _journal;
  DWORDLONG _nextid;
  DWORDLONG _filecount;
  DWORD _starttick, _endtick;

  void ShowRecord(USN_RECORD * record);
  void CheckRecord(USN_RECORD * record);

public:
  int Poll();
};
