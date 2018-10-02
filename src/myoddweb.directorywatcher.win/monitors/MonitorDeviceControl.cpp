#include "MonitorDeviceControl.h"
#include <stdio.h>

MonitorDeviceControl::MonitorDeviceControl(__int64 id) : 
Monitor( id ),
_filecount( 0),
_bytecount(1)
{
  _mft_enum_data = { 0 };
}

void MonitorDeviceControl::ShowRecord(USN_RECORD * record)
{
  void * buffer;
  MFT_ENUM_DATA mft_enum_data;
  DWORD bytecount = 1;
  USN_RECORD * parent_record;

  WCHAR * filename;
  WCHAR * filenameend;

  printf("=================================================================\n");
  printf("RecordLength: %u\n", record->RecordLength);
  printf("MajorVersion: %u\n", (DWORD)record->MajorVersion);
  printf("MinorVersion: %u\n", (DWORD)record->MinorVersion);
  printf("FileReferenceNumber: %lu\n", record->FileReferenceNumber);
  printf("ParentFRN: %lu\n", record->ParentFileReferenceNumber);
  printf("USN: %lu\n", record->Usn);
  printf("Timestamp: %lu\n", record->TimeStamp);
  printf("Reason: %u\n", record->Reason);
  printf("SourceInfo: %u\n", record->SourceInfo);
  printf("SecurityId: %u\n", record->SecurityId);
  printf("FileAttributes: %x\n", record->FileAttributes);
  printf("FileNameLength: %u\n", (DWORD)record->FileNameLength);

  filename = (WCHAR *)(((BYTE *)record) + record->FileNameOffset);
  filenameend = (WCHAR *)(((BYTE *)record) + record->FileNameOffset + record->FileNameLength);

  printf("FileName: %.*ls\n", filenameend - filename, filename);

  buffer = VirtualAlloc(NULL, BUFFER_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

  if (buffer == NULL)
  {
    printf("VirtualAlloc: %u\n", GetLastError());
    return;
  }

  mft_enum_data.StartFileReferenceNumber = record->ParentFileReferenceNumber;
  mft_enum_data.LowUsn = 0;
  mft_enum_data.HighUsn = _maxusn;

  if (!DeviceIoControl(_drive, FSCTL_ENUM_USN_DATA, &mft_enum_data, sizeof(mft_enum_data), buffer, BUFFER_SIZE, &bytecount, NULL))
  {
    printf("FSCTL_ENUM_USN_DATA (show_record): %u\n", GetLastError());
    return;
  }

  parent_record = (USN_RECORD *)((USN *)buffer + 1);

  if (parent_record->FileReferenceNumber != record->ParentFileReferenceNumber)
  {
    printf("=================================================================\n");
    printf("Couldn't retrieve FileReferenceNumber %u\n", record->ParentFileReferenceNumber);
    return;
  }

  ShowRecord(parent_record);
}

void MonitorDeviceControl::CheckRecord(USN_RECORD * record)
{
  WCHAR * filename;
  WCHAR * filenameend;

  filename = (WCHAR *)(((BYTE *)record) + record->FileNameOffset);
  filenameend = (WCHAR *)(((BYTE *)record) + record->FileNameOffset + record->FileNameLength);

  if (filenameend - filename != 8)
  {
    return;
  }

  if (wcsncmp(filename, L"test.txt", 8) != 0)
  {
    return;
  }
  ShowRecord(record);
}

void MonitorDeviceControl::Stop()
{
}

bool MonitorDeviceControl::Poll(const std::wstring& path, bool recursive)
{
  _starttick = GetTickCount();
  printf("Allocating memory.\n");

  _buffer = VirtualAlloc(NULL, BUFFER_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

  if (_buffer == NULL)
  {
    printf("VirtualAlloc: %u\n", GetLastError());
    return false;
  }

  printf("Opening volume.\n");

  _drive = CreateFile(L"\\\\.\\z:", GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);

  if (_drive == INVALID_HANDLE_VALUE)
  {
    printf("CreateFile: %u\n", GetLastError());
    return false;
  }

  printf("Calling FSCTL_QUERY_USN_JOURNAL\n");

  if (!DeviceIoControl(_drive, FSCTL_QUERY_USN_JOURNAL, NULL, 0, _buffer, BUFFER_SIZE, &_bytecount, NULL))
  {
    printf("FSCTL_QUERY_USN_JOURNAL: %u\n", GetLastError());
    return false;
  }

  _journal = (USN_JOURNAL_DATA *)_buffer;

  printf("UsnJournalID: %lu\n", _journal->UsnJournalID);
  printf("FirstUsn: %lu\n", _journal->FirstUsn);
  printf("NextUsn: %lu\n", _journal->NextUsn);
  printf("LowestValidUsn: %lu\n", _journal->LowestValidUsn);
  printf("MaxUsn: %lu\n", _journal->MaxUsn);
  printf("MaximumSize: %lu\n", _journal->MaximumSize);
  printf("AllocationDelta: %lu\n", _journal->AllocationDelta);

  _maxusn = _journal->MaxUsn;

  _mft_enum_data.StartFileReferenceNumber = 0;
  _mft_enum_data.LowUsn = 0;
  _mft_enum_data.HighUsn = _maxusn;

  for (;;)
  {
    //      printf("=================================================================\n");
    //      printf("Calling FSCTL_ENUM_USN_DATA\n");

    if (!DeviceIoControl(_drive, FSCTL_ENUM_USN_DATA, &_mft_enum_data, sizeof(_mft_enum_data), _buffer, BUFFER_SIZE, &_bytecount, NULL))
    {
      printf("=================================================================\n");
      printf("FSCTL_ENUM_USN_DATA: %u\n", GetLastError());
      printf("Final ID: %lu\n", _nextid);
      printf("File count: %lu\n", _filecount);
      _endtick = GetTickCount();
      printf("Ticks: %u\n", _endtick - _starttick);
      return false;
    }

    //      printf("Bytes returned: %u\n", bytecount);

    _nextid = *((DWORDLONG *)_buffer);
    //      printf("Next ID: %lu\n", nextid);

    _record = (USN_RECORD *)((USN *)_buffer + 1);
    _recordend = (USN_RECORD *)(((BYTE *)_buffer) + _bytecount);

    while (_record < _recordend)
    {
      _filecount++;
      CheckRecord(_record);
      _record = (USN_RECORD *)(((BYTE *)_record) + _record->RecordLength);
    }

    _mft_enum_data.StartFileReferenceNumber = _nextid;
  }
  return true;
}