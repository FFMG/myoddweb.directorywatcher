#include "MonitorsManagerTestHelper.h"

#include "windows.h"
#include <iostream>
#include <fstream>  
#include <condition_variable>
#include <thread>
#include <chrono>
#include <map>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>

#include "../myoddweb.directorywatcher.win/utils/Io.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"

using myoddweb::directorywatcher::EventAction;
using myoddweb::directorywatcher::Io;
using myoddweb::directorywatcher::Wait;

std::mutex _cv_m;

void _wait(long long ms)
{
  static std::condition_variable cv;
  std::unique_lock<std::mutex> lk(_cv_m);
  auto now = std::chrono::system_clock::now();
  cv.wait_until(lk, now + std::chrono::milliseconds(ms), []() {return false; });
}

class MonitorsManagerTestHelper;
std::map<long long, MonitorsManagerTestHelper*> _managers;

MonitorsManagerTestHelper* Get(long long id)
{
  std::unique_lock<std::mutex> lk(_cv_m);
  const auto it = _managers.find(id);
  if (it == _managers.end())
  {
    return nullptr;
  }
  return it->second;
}

bool Remove(long long id)
{
  std::unique_lock<std::mutex> lk(_cv_m);
  const auto it = _managers.find(id);
  if (it == _managers.end())
  {
    return false;
  }
  _managers.erase(it);
  return true;
}

bool Add(long long id, MonitorsManagerTestHelper* mng)
{
  std::unique_lock<std::mutex> lk(_cv_m);
  _managers[id] = mng;
  return true;
}

MonitorsManagerTestHelper::MonitorsManagerTestHelper() :
  _addedFiles(0),
  _addedFolders(0),
  _removedFiles(0),
  _removedFolders(0)
{
  wchar_t lpTempPathBuffer[MAX_PATH];
  const auto ret = GetTempPathW(MAX_PATH, lpTempPathBuffer);
  if (ret > MAX_PATH || (ret == 0))
  {
    throw std::exception("Could not get temp folder.");
  }
  _tmpFolder = lpTempPathBuffer;
  auto subDirectory = L"test." + RandomString(4);
  _folder = ::Io::Combine( lpTempPathBuffer, subDirectory );

  CreateDirectoryW(Folder(), nullptr);
}

MonitorsManagerTestHelper::~MonitorsManagerTestHelper()
{
  for each (auto file in _files)
  {
    RemoveFile(file);
  }

  for each (auto folder in _folders)
  {
    RemoveDirectoryW(folder.c_str());
  }

  // and the directory
  RemoveDirectoryW(Folder());
}

const wchar_t* MonitorsManagerTestHelper::Folder() const 
{
  return _folder.c_str();
}

void MonitorsManagerTestHelper::EventAction(const ::EventAction action, bool isFile)
{
  switch (action)
  {
  case EventAction::Added:
    if (isFile) {
      ++_addedFiles;
    }
    else {
      ++_addedFolders;
    }
    break;

  case EventAction::Removed:
    if (isFile) {
      ++_removedFiles;
    }
    else {
      ++_removedFolders;
    }
    return;

  default:
    break;
  }
}

int MonitorsManagerTestHelper::Added( bool isFile ) const
{
  return isFile ? _addedFiles: _addedFolders;
}

int MonitorsManagerTestHelper::Removed( bool isFile ) const
{
  return isFile ? _removedFiles : _removedFolders;
}

bool MonitorsManagerTestHelper::RemoveFolder(const std::wstring& folder)
{
  if (0 == RemoveDirectoryW(folder.c_str()))
  {
    return false;
  }
  return true;
}

bool MonitorsManagerTestHelper::RemoveFile(const std::wstring& filename)
{
  if (0 == DeleteFileW(filename.c_str()))
  {
    return false;
  }
  return true;
}

std::wstring MonitorsManagerTestHelper::AddFile()
{
  for (;;)
  {
    auto filename = ::Io::Combine(Folder(), RandomString(8));
    filename += L".txt";

    std::ifstream f(filename.c_str());
    if (f.good())
    {
      continue;
    }

    std::ofstream outfile(filename.c_str());
    outfile << L"my text here!" << std::endl;
    outfile.close();

    _files.push_back(filename);
    return filename;
  }
}

std::wstring MonitorsManagerTestHelper::AddFolder()
{
  for (;;)
  {
    auto folder = ::Io::Combine(Folder(), RandomString(6));

    struct _stati64 info = { 0 };

    if (_wstat64(folder.c_str(), &info) == 0)
    {
      if (info.st_mode & S_IFDIR)
      {
        continue;
      }
    }
    CreateDirectoryW(folder.c_str(), nullptr);

    _folders.push_back(folder);
    return folder;
  }
}

std::wstring MonitorsManagerTestHelper::RandomString(const size_t length)
{
  const auto randchar = []() -> wchar_t
  {
    const wchar_t charset[] =
      L"0123456789"
      L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      L"abcdefghijklmnopqrstuvwxyz";
    const auto maxIndex = (wcslen(charset) - 1);
    return charset[rand() % maxIndex];
  };
  std::wstring str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}
