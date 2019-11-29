#include "pch.h"

#include "../myoddweb.directorywatcher.win/utils/MonitorsManager.h"
#include "../myoddweb.directorywatcher.win/utils/Io.h"

using myoddweb::directorywatcher::MonitorsManager;
using myoddweb::directorywatcher::Request;
using myoddweb::directorywatcher::Io;
using myoddweb::directorywatcher::EventCallback;

class MonitorsManagerTestHelper
{
private:
  std::wstring _folder;
  std::wstring _tmpFolder;
  
public:
  MonitorsManagerTestHelper()
  {
    //wchar_t lpTempPathBuffer[MAX_PATH];
    //const auto ret = GetTempPathW(MAX_PATH, lpTempPathBuffer);
    //if (ret > MAX_PATH || (ret == 0))
    //{
    //  throw std::exception("Could not get temp folder.");
    //}
    //_tmpFolder = lpTempPathBuffer;
    //_folder = ::Io::Combine(RandomString(4), lpTempPathBuffer);
  }

  const wchar_t* Folder() const {
    return _folder.c_str();
  }
    
protected:
  std::wstring RandomString( const size_t length)
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

TEST(MonitorsManager, CallbackWhenFileIsAdded) {

  auto z = 20;

  // create the helper.
  const auto helper = MonitorsManagerTestHelper();

  auto count = 0;
  // monitor that folder.
  auto f = [](
    const long long id,
    const bool isFile,
    const wchar_t* name,
    const wchar_t* oldName,
    const int action,
    const int error,
    const long long dateTimeUtc
    ) -> int
  {
    return 0;
  };
  const auto request = ::Request(helper.Folder(), false, f, 0);
  //const auto id = ::MonitorsManager::Start(request);

  // add a single file to it.
  //helper.AddFile();

  //EXPECT_NO_THROW(::MonitorsManager::Stop(id));
}
