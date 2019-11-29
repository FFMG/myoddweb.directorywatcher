#include "MonitorsManagerTestHelper.h"

#include "windows.h"
#include <iostream>
#include <fstream>  
#include <condition_variable>
#include <thread>
#include <chrono>
#include <map>
#include <algorithm>

#include "../myoddweb.directorywatcher.win/utils/Io.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"

using myoddweb::directorywatcher::EventAction;
using myoddweb::directorywatcher::Io;

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
    _added(0),
    _removed(0)
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

  // and the directory
  RemoveDirectoryW(Folder());
}

void MonitorsManagerTestHelper::Wait(long long ms)
{
  std::thread t(_wait, ms);
  t.join();
}

const wchar_t* MonitorsManagerTestHelper::Folder() const 
{
  return _folder.c_str();
}

void MonitorsManagerTestHelper::EventAction(const ::EventAction action)
{
  switch (action)
  {
  case EventAction::Added:
    ++_added;
    break;

  case EventAction::Removed:
    ++_removed;
    return;

  default:
    break;
  }
}

int MonitorsManagerTestHelper::Added() const
{
  return _added;
}

int MonitorsManagerTestHelper::Removed() const
{
  return _removed;
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
