#pragma once
#include "pch.h"

#include "../myoddweb.directorywatcher.win/utils/MonitorsManager.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"

#include "MonitorsManagerTestHelper.h"

using myoddweb::directorywatcher::Wait;
using myoddweb::directorywatcher::EventAction;
using myoddweb::directorywatcher::MonitorsManager;
using myoddweb::directorywatcher::Request;
using myoddweb::directorywatcher::EventCallback;

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

  const auto request1 = ::Request(helper->Folder(), recursive, function, TEST_TIMEOUT);
  const auto request2 = ::Request(helper->Folder(), recursive, function, TEST_TIMEOUT);
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

  const auto request1 = ::Request(helper1->Folder(), recursive, function, TEST_TIMEOUT);
  const auto request2 = ::Request(helper2->Folder(), recursive, function, TEST_TIMEOUT);
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
