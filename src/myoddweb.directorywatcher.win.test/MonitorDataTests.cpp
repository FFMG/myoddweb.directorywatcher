#include "pch.h"

#include <filesystem>
#include <fstream>

#include "../myoddweb.directorywatcher.win/monitors/win/Data.h"
#include "../myoddweb.directorywatcher.win/monitors/WinMonitor.h"
#include "../myoddweb.directorywatcher.win/utils/Threads/WorkerPool.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"
#include "../myoddweb.directorywatcher.win/utils/Io.h"
#include "MonitorsManagerTestHelper.h"

using myoddweb::directorywatcher::win::Data;
using myoddweb::directorywatcher::WinMonitor;
using myoddweb::directorywatcher::Request;
using myoddweb::directorywatcher::threads::WorkerPool;
using myoddweb::directorywatcher::Wait;
using myoddweb::directorywatcher::Io;

namespace
{
  constexpr unsigned long kTestBufferLength = 64 * 1024;

  std::wstring make_temp_folder()
  {
    const auto tmpFolder = std::filesystem::temp_directory_path().wstring();
    const auto folder = Io::combine(tmpFolder, L"moddw_data_test_" + std::to_wstring(GetTickCount64()) + L"_" + std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(rand()));
    std::filesystem::create_directory(folder);
    return folder;
  }
}

class DataTestHelper
{
public:
  static void process_error(Data& data, const unsigned long errorCode)
  {
    data.process_error(errorCode);
  }

  static void file_io_completion_routine(const unsigned long dwErrorCode, const unsigned long dwNumberOfBytesTransfered, _OVERLAPPED* lpOverlapped)
  {
    Data::file_io_completion_routine(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
  }

  static unsigned char* clone(const Data& data, const unsigned long ulSize)
  {
    return data.clone(ulSize);
  }

  static void clear_buffer(Data& data)
  {
    data.clear_buffer();
  }

  static void stop_and_wait(Data& data)
  {
    data.stop_and_wait();
  }
};

TEST(MonitorData, CompletionRoutineWithNullDataDoesNotCrash)
{
  struct FakeOverlappedData : OVERLAPPED
  {
    Data* pdata;
  };

  FakeOverlappedData overlapped{};
  overlapped.pdata = nullptr;

  EXPECT_NO_THROW(
    DataTestHelper::file_io_completion_routine(ERROR_SUCCESS, 0, &overlapped)
  );
}

TEST(MonitorData, UnhandledErrorCodeReArmsListening)
{
  auto pool = WorkerPool(10);

  const auto folder = make_temp_folder();

  Data data(1, folder.c_str(), FILE_NOTIFY_CHANGE_FILE_NAME, false, kTestBufferLength, pool);
  ASSERT_TRUE(data.start());

  DataTestHelper::process_error(data, ERROR_MORE_DATA);

  const auto file = Io::combine(folder, L"test.txt");
  std::ofstream outfile(file);
  outfile << "hello";
  outfile.close();

  auto sawChange = false;
  Wait::spin_until([&]
    {
      auto cloned = data.get();
      if (!cloned.empty())
      {
        sawChange = true;
      }
      for (const auto& raw : cloned)
      {
        delete[] raw;
      }
      return sawChange;
    }, TEST_TIMEOUT_WAIT);

  EXPECT_TRUE(sawChange);

  data.stop();
  std::filesystem::remove_all(folder);
}

TEST(MonitorData, CheckStillValidDoesNotResurrectAfterStop)
{
  auto pool = WorkerPool(10);
  const auto folder = make_temp_folder();

  Data data(1, folder.c_str(), FILE_NOTIFY_CHANGE_FILE_NAME, false, kTestBufferLength, pool);
  ASSERT_TRUE(data.start());

  // fully stop, (synchronously waiting for the async handle/buffer cleanup
  DataTestHelper::stop_and_wait(data);

  data.check_still_valid();
  data.check_still_valid();

  // give a (buggy, resurrected) watch every chance to pick up a new file.
  const auto file = Io::combine(folder, L"after_stop.txt");
  std::ofstream outfile(file);
  outfile << "hello";
  outfile.close();

  auto sawChange = false;
  Wait::spin_until([&]
    {
      auto cloned = data.get();
      if (!cloned.empty())
      {
        sawChange = true;
      }
      for (const auto& raw : cloned)
      {
        delete[] raw;
      }
      return sawChange;
    }, TEST_TIMEOUT_WAIT);

  EXPECT_FALSE(sawChange);

  std::filesystem::remove_all(folder);
}

TEST(MonitorData, CloneReturnsNullWithoutAllocatingWhenBufferIsNull)
{
  auto pool = WorkerPool(10);
  const auto folder = make_temp_folder();

  Data data(1, folder.c_str(), FILE_NOTIFY_CHANGE_FILE_NAME, false, kTestBufferLength, pool);
  ASSERT_TRUE(data.start());

  DataTestHelper::clear_buffer(data);

  const auto cloned = DataTestHelper::clone(data, 16);
  EXPECT_EQ(nullptr, cloned);

  data.stop();
  std::filesystem::remove_all(folder);
}
