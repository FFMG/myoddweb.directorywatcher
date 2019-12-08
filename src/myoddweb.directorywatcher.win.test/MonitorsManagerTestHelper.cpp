#include "MonitorsManagerTestHelper.h"

#include <filesystem>
#include <fstream>  
#include <map>
#include <algorithm>

#include "../myoddweb.directorywatcher.win/utils/Io.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"
#include <gtest\gtest.h>

#include <locale>
#include <codecvt>
#include <string>

using myoddweb::directorywatcher::EventAction;
using myoddweb::directorywatcher::Io;
using myoddweb::directorywatcher::Wait;

std::mutex _cv_m;

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

std::wstring utf8toUtf16(const std::string& str)
{
  if (str.empty())
    return std::wstring();

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return converter.from_bytes(str);
}

MonitorsManagerTestHelper::MonitorsManagerTestHelper() :
  _addedFiles(0),
  _addedFolders(0),
  _removedFiles(0),
  _removedFolders(0)
{
  const auto name = utf8toUtf16(
    ::testing::UnitTest::GetInstance()->current_test_info()->name()
  );

  _tmpFolder = std::filesystem::temp_directory_path();
  
  auto subDirectory = L"test." + name + RandomString(4);
  _folder = ::Io::Combine(_tmpFolder, subDirectory );

  std::filesystem::create_directory(Folder());
}

/**
 * \brief 
 */
MonitorsManagerTestHelper::~MonitorsManagerTestHelper()
{
  for each (auto file in _files)
  {
    RemoveFile(file);
  }

  for each (auto folder in _folders)
  {
    try
    {
      std::filesystem::remove(folder);
    }
    catch (...)
    {
      // to log
    }
  }
  
  try
  {
    // and the directory
    std::filesystem::remove(Folder());
  }
  catch (...)
  {
    // to log
  }
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

int MonitorsManagerTestHelper::Added( const bool isFile ) const
{
  return isFile ? _addedFiles: _addedFolders;
}

int MonitorsManagerTestHelper::Removed( const bool isFile ) const
{
  return isFile ? _removedFiles : _removedFolders;
}

bool MonitorsManagerTestHelper::RemoveFolder(const std::wstring& folder) const
{
  return std::filesystem::remove(folder);
}

bool MonitorsManagerTestHelper::RemoveFile( const std::wstring& filename) const
{
  try
  {
    return std::filesystem::remove(filename);
  }
  catch (...)
  {
    return false;
  }
}

std::wstring MonitorsManagerTestHelper::AddFile()
{
  for (;;)
  {
    auto filename = ::Io::Combine(Folder(), RandomString(8));
    filename += L".txt";

    // does it exist already?
    if (std::filesystem::exists(filename))
    {
      continue;
    }

    std::ofstream outfile(filename.c_str());
    outfile << L"my text here!" << std::endl;
    outfile.close();

    // was it created properly?
    if( !std::filesystem::exists( filename ))
    {
      continue;
    }

    _files.push_back(filename);
    return filename;
  }
}

std::wstring MonitorsManagerTestHelper::AddFolder()
{
  for (;;)
  {
    auto folder = ::Io::Combine(Folder(), RandomString(6));

    if (std::filesystem::exists(folder))
    {
      continue;
    }


    std::filesystem::create_directory( folder );
    if (!std::filesystem::exists(folder))
    {
      continue;
    }

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
