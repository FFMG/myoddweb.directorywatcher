#include "pch.h"

#include "../myoddweb.directorywatcher.win/utils/MonitorsManager.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"

#include "MonitorsManagerTestHelper.h"
#include "RequestTestHelper.h"

using myoddweb::directorywatcher::Wait;
using myoddweb::directorywatcher::EventAction;
using myoddweb::directorywatcher::MonitorsManager;
using myoddweb::directorywatcher::Request;
using myoddweb::directorywatcher::EventCallback;

typedef std::tuple<int, bool> IdentifierParams;
class ValidateNumberOfItemDeleted :public ::testing::TestWithParam<IdentifierParams> {};

INSTANTIATE_TEST_SUITE_P(
  MonitorsManagerDelete,
  ValidateNumberOfItemDeleted,
  testing::Combine(
    ::testing::Values(0, 1, 20, 42),
    ::testing::Values(true, false)
  ));
TEST(MonitorsManagerDelete, IfTimeoutIsZeroCallbackIsNeverCalled) {
    // create the helper.
    auto helper = new MonitorsManagerTestHelper();

    // use the test request to create the Request
    // we make a copy of our helper onto the 'real' request to make sure copy is not broken
    const auto r = RequestHelper(
      helper->Folder(),
      false,
      nullptr,
      eventFunction,
      nullptr,
      0,
      50);

    // monitor that folder.
    const auto request = ::Request(r);
    const auto id = ::MonitorsManager::Start(request);

    Wait::SpinUntil([] { return ::MonitorsManager::Ready(); }, TEST_TIMEOUT_WAIT);

    Add(id, helper);

    // add a single file to it.
    const auto file = helper->AddFile();

    // delete it
    ASSERT_TRUE(helper->RemoveFile(file));

    // wait a bit to give a chance for invalid files to be reported.
    Wait::Delay(1000);

    ASSERT_EQ(0, helper->Added(true));
    ASSERT_EQ(0, helper->Removed(true));

    ASSERT_NO_THROW(::MonitorsManager::Stop(id));

    ASSERT_TRUE(Remove(id));
    delete helper;
}

TEST_P(ValidateNumberOfItemDeleted, CallbackWhenFileIsDeleted) {

  for (int i = 0; i < 10; ++i)
  {
MYODDWEB_OUT("---------------------------\n");
    try
    {
      // create the helper.
      auto helper = new MonitorsManagerTestHelper();

      const auto number = std::get<0>(GetParam());
      const auto recursive = std::get<1>(GetParam());

      auto count = 0;

      // use the test request to create the Request
      // we make a copy of our helper onto the 'real' request to make sure copy is not broken
      const auto r = RequestHelper(
        helper->Folder(),
        recursive,
        nullptr,
        eventFunction,
        nullptr,
        TEST_TIMEOUT,
        0);

      // monitor that folder.
      const auto request = ::Request(r);
      const auto id = ::MonitorsManager::Start(request);
      Add(id, helper);

      // wait for the pool to start
      if (!Wait::SpinUntil([&]
        {
          return ::MonitorsManager::Ready();
        }, TEST_TIMEOUT_WAIT))
      {
        GTEST_FATAL_FAILURE_("Unable to start pool");
      }

      auto files = std::vector<std::wstring>();
      for (auto i = 0; i < number; ++i)
      {
        // add a single file to it.
        files.push_back(helper->AddFile());
      }
      // delete them all
      for each (auto file in files)
      {
        ASSERT_TRUE(helper->RemoveFile(file));
      }

      // give a little more than the timeout
      if( !Wait::SpinUntil(
        [&] {
          return number == helper->Removed(true);
        }, TEST_TIMEOUT_WAIT) )
      {
        MonitorsManagerTestHelper::LoggerFunction(id, 0, L"wtf");
      }

      ASSERT_EQ(number, helper->Removed(true));

      ASSERT_NO_THROW(::MonitorsManager::Stop(id));

      ASSERT_TRUE(Remove(id));
      delete helper;
    }
    catch (const std::exception& e)
    {
      MYODDWEB_OUT( "A" );
    }
  }
}

TEST_P(ValidateNumberOfItemDeleted, CallbackWhenFolderIsDeleted) {
  
  // create the helper.
  auto helper = new MonitorsManagerTestHelper();

  const auto number = std::get<0>(GetParam());
  const auto recursive = std::get<1>(GetParam());

  auto count = 0;

  // use the test request to create the Request
  // we make a copy of our helper onto the 'real' request to make sure copy is not broken
  const auto r = RequestHelper(
    helper->Folder(),
    recursive,
    nullptr,
    eventFunction,
    nullptr,
    TEST_TIMEOUT,
    0);

  // monitor that folder.
  const auto request = ::Request(r);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, helper);

  // wait for the pool to start
  if (!Wait::SpinUntil([&]
    {
      return ::MonitorsManager::Ready();
    }, TEST_TIMEOUT_WAIT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  auto folders = std::vector<std::wstring>();
  for (auto i = 0; i < number; ++i)
  {
    // add a single file to it.
    folders.push_back(helper->AddFolder());
  }

  // delete them all
  for each (auto folder in folders)
  {
    ASSERT_TRUE(helper->RemoveFolder(folder));
  }

  // give a little more than the timeout
  Wait::SpinUntil(
    [&] {
      return number == helper->Removed(false);
    }, TEST_TIMEOUT_WAIT);

  ASSERT_EQ(number, helper->Removed(false));

  ASSERT_NO_THROW(::MonitorsManager::Stop(id));

  ASSERT_TRUE(Remove(id));
  delete helper;
}