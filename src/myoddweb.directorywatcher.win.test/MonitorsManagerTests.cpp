#include "pch.h"

#include "windows.h"
#include <iostream>
#include <fstream>  
#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>

#include "../myoddweb.directorywatcher.win/utils/MonitorsManager.h"
#include "../myoddweb.directorywatcher.win/utils/Io.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"

using myoddweb::directorywatcher::EventAction;
using myoddweb::directorywatcher::MonitorsManager;
using myoddweb::directorywatcher::Request;
using myoddweb::directorywatcher::Io;
using myoddweb::directorywatcher::EventCallback;

std::mutex _cv_m;

void _wait(long long ms)
{
  static std::condition_variable cv;
  std::unique_lock<std::mutex> lk(_cv_m);
  auto now = std::chrono::system_clock::now();
  cv.wait_until(lk, now + std::chrono::milliseconds(ms), []() {return false; });
};

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

class MonitorsManagerTestHelper
{
private:
  std::wstring _folder;
  std::wstring _tmpFolder;
  int _added;

public:
  MonitorsManagerTestHelper() : 
    _added(0)
  {
    wchar_t lpTempPathBuffer[MAX_PATH];
    const auto ret = GetTempPathW(MAX_PATH, lpTempPathBuffer);
    if (ret > MAX_PATH || (ret == 0))
    {
      throw std::exception("Could not get temp folder.");
    }
    _tmpFolder = lpTempPathBuffer;
    _folder = ::Io::Combine(lpTempPathBuffer, RandomString(4));

    CreateDirectoryW(Folder(), nullptr);
  }

  void Wait(long long ms)
  {
    std::thread t( _wait , ms);
    t.join();
  }

  const wchar_t* Folder() const {
    return _folder.c_str();
  }

  void EventAction(EventAction action)
  {
    switch (action)
    {
    case EventAction::Added:
      ++_added;
      break;

    default:
      break;
    }
  }

  int Added() const
  {
    return _added;
  }

  void AddFile() const
  {
    auto filename = ::Io::Combine(Folder(), RandomString(8));
    filename += L".txt";

    std::ofstream outfile( filename.c_str() );
    outfile << L"my text here!" << std::endl;
    outfile.close();
  }
    
protected:
  static std::wstring RandomString( const size_t length)
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
};

auto function = []
(
  const long long id,
  const bool isFile,
  const wchar_t* name,
  const wchar_t* oldName,
  const int action,
  const int error,
  const long long dateTimeUtc
  ) -> int
{
  Get(id)->EventAction((EventAction)action);
  return 0;
};


TEST(MonitorsManager, SimpleStartAndStop) {

  const auto request = ::Request(L"c:\\", false, nullptr, 0);
  const auto id = ::MonitorsManager::Start( request );

  // do nothing ...

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));
}

TEST(MonitorsManager, InvalidPathDoesNOtThrow) {

  const auto request = ::Request(L"somebadname", false, nullptr, 0);
  const auto id = ::MonitorsManager::Start(request);

  // do nothing ...

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));
}

TEST(MonitorsManager, IfTimeoutIsZeroCallbackIsNeverCalled) {
  // create the helper.
  auto helper = new MonitorsManagerTestHelper();

  auto count = 0;
  // monitor that folder.
  const auto request = ::Request(helper->Folder(), false, function, 0);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, helper);

  // add a single file to it.
  helper->AddFile();

  helper->Wait(1000);

  EXPECT_EQ(0, helper->Added());

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));

  EXPECT_TRUE( Remove(id) );
  delete helper;
}

TEST(MonitorsManager, CallbackWhenFileIsAdded) {
  auto list = { 1, 42, 100 };
  for (auto loop = 0; loop < list.size(); ++loop)
  {
    // create the helper.
    auto helper = new MonitorsManagerTestHelper();

    auto count = 0;
    // monitor that folder.
    const auto request = ::Request(helper->Folder(), false, function, 50);
    const auto id = ::MonitorsManager::Start(request);
    Add(id, helper);

    const auto number = list.begin() + loop;
    for (auto i = 0; i < *number; ++i)
    {
      // add a single file to it.
      helper->AddFile();
    }
    helper->Wait(1000);

    EXPECT_EQ(*number, helper->Added());

    EXPECT_NO_THROW(::MonitorsManager::Stop(id));

    EXPECT_TRUE(Remove(id));
    delete helper;
  }
}
