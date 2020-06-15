#include "pch.h"

#include "../myoddweb.directorywatcher.win/utils/MonitorsManager.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"

#include "MonitorsManagerTestHelper.h"
#include "RequestTestHelper.h"

using myoddweb::directorywatcher::Wait;
using myoddweb::directorywatcher::EventAction;
using myoddweb::directorywatcher::MonitorsManager;

class RecursiveAndNonRecursive :public ::testing::TestWithParam<bool> {};
INSTANTIATE_TEST_SUITE_P(
  MonitorsManagerEdge,
  RecursiveAndNonRecursive,
  ::testing::Values(true, false)
  );

TEST_P(RecursiveAndNonRecursive, TwoWatchersOnTheSameFolder) {

  // create the helper.
  auto helper = new MonitorsManagerTestHelper();

  const auto recursive = GetParam();

  //  use the test request to create the Request
  const auto r = RequestHelper(
    helper->Folder(),
    recursive,
    nullptr,
    function, 
    nullptr,
    TEST_TIMEOUT,
    0);

  const auto request1 = ::Request(r);
  const auto request2 = ::Request(r);
  const auto id1 = ::MonitorsManager::Start(request1);
  const auto id2 = ::MonitorsManager::Start(request2);
  Add(id1, helper);
  Add(id2, helper);

  // wait for the thread to get started
  Wait::Delay(TEST_TIMEOUT_WAIT);

  const auto number = 10;
  for (auto i = 0; i < number; ++i)
  {
    auto _ = helper->AddFile();
    Wait::Delay(1);
  }

  // wait a little
  // give a little more than the timeout
  Wait::SpinUntil(
    [&] {
      return 2*number == helper->Added(true);
    }, 2 * number * TEST_TIMEOUT);

  // we should have our file.
  EXPECT_EQ(2*number, helper->Added(true));

  EXPECT_NO_THROW(::MonitorsManager::Stop(id1));
  EXPECT_NO_THROW(::MonitorsManager::Stop(id2));

  delete helper;
}

TEST_P(RecursiveAndNonRecursive, TwoWatchersOnTwoSeparateFolders) {

  // create the helper.
  auto helper1 = new MonitorsManagerTestHelper();
  auto helper2 = new MonitorsManagerTestHelper();

  const auto recursive = GetParam();

  //  use the test request to create the Reques
  const auto r1 = RequestHelper(
    helper1->Folder(),
    recursive,
    nullptr,
    function,
    nullptr,
    TEST_TIMEOUT,
    0);

  const auto r2 = RequestHelper(
    helper1->Folder(),
    recursive,
    nullptr,
    function,
    nullptr,
    TEST_TIMEOUT,
    0);

  const auto request1 = ::Request(r1);
  const auto request2 = ::Request(r2);
  const auto id1 = ::MonitorsManager::Start(request1);
  const auto id2 = ::MonitorsManager::Start(request2);

  Add(id1, helper1);
  Add(id2, helper2);

  // wait for the thread to get started
  Wait::Delay(TEST_TIMEOUT_WAIT);

  // just add a file
  const auto number = 10;
  for (auto i = 0; i < number; ++i)
  {
    auto _ = helper1->AddFile();
    _ = helper2->AddFile();
  }

  // wait a little
  // give a little more than the timeout
  Wait::SpinUntil(
    [&] {
      return number == helper1->Added(true) && number == helper2->Added(true);
    }, 2*number*TEST_TIMEOUT);

  // we should have our file.
  EXPECT_EQ(number, helper1->Added(true));
  EXPECT_EQ(number, helper2->Added(true));

  EXPECT_NO_THROW(::MonitorsManager::Stop(id1));
  EXPECT_NO_THROW(::MonitorsManager::Stop(id2));

  delete helper1;
  delete helper2;
}

TEST(MonitorsManagerEdgeCases, StartAndStopAlmostInstantly) {

  // create the helper.
  auto helper = MonitorsManagerTestHelper();
  const auto recursive = true;

  // use the test request to create the Request
  // we make a copy of our helper onto the 'real' request to make sure copy is not broken
  const auto r = RequestHelper(
    helper.Folder(),
    recursive,
    nullptr,
    function,
    nullptr,
    TEST_TIMEOUT,
    0);

  // monitor that folder.
  const auto request = ::Request(r);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, &helper);

  // wait for the thread to get started
  Wait::Delay(TEST_TIMEOUT_WAIT);

  auto folders = std::vector<std::wstring>();

  // add a folder and remove it
  const auto folder = helper.AddFolder();
  ASSERT_TRUE(helper.RemoveFolder(folder));

  // no waiting for anything

  // stop
  ASSERT_NO_THROW(::MonitorsManager::Stop(id));

  // all done
  ASSERT_TRUE(Remove(id));
}