#pragma once
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
class ValidateNumberOfItemAdded :public ::testing::TestWithParam<IdentifierParams> {};

INSTANTIATE_TEST_SUITE_P(
  MonitorsManagerAdd,
  ValidateNumberOfItemAdded,
  testing::Combine(
    ::testing::Values(0, 1, 17, 42),
    ::testing::Values(true, false)
  ));

TEST(MonitorsManagerAdd, SimpleStartAndStop) {

  // use the test request to create the Request
  // we make a copy of our helper onto the 'real' request to make sure copy is not broken
  const auto r = RequestHelper(
    L"c:\\",
    false,
    nullptr,
    nullptr,
    nullptr,
    50,
    0);
  const auto request = ::Request(r);
  const auto id = ::MonitorsManager::Start( request );

  // do nothing ...

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));
}

TEST(MonitorsManagerAdd, StoppingWhenWeNeverStarted) {

  // do nothing ...
  // and stop something that was never started.
  EXPECT_NO_THROW(::MonitorsManager::Stop(42));
}

TEST(MonitorsManagerAdd, StoppingWhatWasNeverStarted) {

  // use the test request to create the Request
  // we make a copy of our helper onto the 'real' request to make sure copy is not broken
  const auto r = RequestHelper(
    L"c:\\",
    false,
    nullptr,
    nullptr,
    nullptr,
    50,
    0);

  const auto request = ::Request(r);
  const auto id = ::MonitorsManager::Start(request);

  // do nothing ...

  // stop the wrong one
  EXPECT_NO_THROW(::MonitorsManager::Stop(id + 1));

  // stop the correct one
  EXPECT_NO_THROW(::MonitorsManager::Stop(id));
}

TEST(MonitorsManagerAdd, StartStopThenAddFileToFolder) {
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
      50,
      0);

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

    // just add a file
    auto _ = helper->AddFile();

    EXPECT_NO_THROW(::MonitorsManager::Stop(id));

    // this should not throw as we are not watching anything anymore
    _ = helper->AddFile();
}

TEST(MonitorsManagerAdd, InvalidPathDoesNOtThrow) {

  // use the test request to create the Request
  // we make a copy of our helper onto the 'real' request to make sure copy is not broken
  const auto r = RequestHelper(
    L"somebadname",
    false,
    nullptr,
    nullptr,
    nullptr,
    0,
    0);
  const auto request = ::Request(r);
  const auto id = ::MonitorsManager::Start(request);

  // do nothing ...

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));
}

TEST(MonitorsManagerAdd, IfTimeoutIsZeroCallbackIsNeverCalled) {
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
    0);

  auto count = 0;
  // monitor that folder.
  const auto request = ::Request(r);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, helper);

  // add a single file to it.
  helper->AddFile();

  // wait a bit to give a chance for invalid files to be reported.
  // wait for the pool to start
  if (!Wait::SpinUntil([&]
    {
      return ::MonitorsManager::Ready();
    }, TEST_TIMEOUT_WAIT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  EXPECT_EQ(0, helper->Added(true));

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));

  EXPECT_TRUE( Remove(id) );
  delete helper;
}

TEST_P(ValidateNumberOfItemAdded, CallbackWhenFileIsAdded) {
  
  // create the helper.
  auto helper = new MonitorsManagerTestHelper();

  // monitor that folder.
  const auto number = std::get<0>(GetParam());
  const auto recursive = std::get<1>(GetParam());

  // use the test request to create the Request
    // we make a copy of our helper onto the 'real' request to make sure copy is not broken
  const auto r = RequestHelper(
    helper->Folder(),
    recursive,
    nullptr,
    eventFunction,
    nullptr,
    50,
    0);

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

  for (auto i = 0; i < number; ++i)
  {
    // add a single file to it.
    auto _ = helper->AddFile();
  }

  // give a little more than the timeout
  Wait::SpinUntil(
    [&] {
      return number == helper->Added(true);
    }, TEST_TIMEOUT_WAIT);

  EXPECT_EQ(number, helper->Added(true));

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));

  EXPECT_TRUE(Remove(id));
  delete helper;
}

TEST_P(ValidateNumberOfItemAdded, CallbackWhenFolderIsAdded) {
  
  // create the helper.
  auto helper = new MonitorsManagerTestHelper();

  auto count = 0;
  // monitor that folder.
  const auto number = std::get<0>(GetParam());
  const auto recursive = std::get<1>(GetParam());

  // use the test request to create the Request
    // we make a copy of our helper onto the 'real' request to make sure copy is not broken
  const auto r = RequestHelper(
    helper->Folder(),
    recursive,
    nullptr,
    eventFunction,
    nullptr,
    50,
    0);

  const auto request = ::Request(r);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, helper);

  // wait for the thread to get started
  Wait::Delay(TEST_TIMEOUT_WAIT);

  for (auto i = 0; i < number; ++i)
  {
    // add a single file to it.
    helper->AddFolder();
  }

  // give a little more than the timeout
  Wait::SpinUntil(
    [&] {
      return number == helper->Added(false);
    }, TEST_TIMEOUT_WAIT);

  EXPECT_EQ(number, helper->Added(false));

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));

  EXPECT_TRUE(Remove(id));
  delete helper;
}