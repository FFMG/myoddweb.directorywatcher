#pragma once

#include "pch.h"

#include "../myoddweb.directorywatcher.win/utils/MonitorsManager.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"

#include "MonitorsManagerTestHelper.h"

using myoddweb::directorywatcher::EventAction;
using myoddweb::directorywatcher::MonitorsManager;
using myoddweb::directorywatcher::Request;
using myoddweb::directorywatcher::EventCallback;

class ValidateNumberOfFilesAdded :public ::testing::TestWithParam<int> {
protected:
  int numberOfFiles;
};

TEST(MonitorsManagerAdd, SimpleStartAndStop) {

  const auto request = ::Request(L"c:\\", false, nullptr, 0);
  const auto id = ::MonitorsManager::Start( request );

  // do nothing ...

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));
}

TEST(MonitorsManagerAdd, InvalidPathDoesNOtThrow) {

  const auto request = ::Request(L"somebadname", false, nullptr, 0);
  const auto id = ::MonitorsManager::Start(request);

  // do nothing ...

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));
}

TEST(MonitorsManagerAdd, IfTimeoutIsZeroCallbackIsNeverCalled) {
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

TEST_P(ValidateNumberOfFilesAdded, CallbackWhenFileIsAdded) {
  const auto timeout = 50;
  // create the helper.
  auto helper = new MonitorsManagerTestHelper();

  auto count = 0;
  // monitor that folder.
  const auto request = ::Request(helper->Folder(), false, function, timeout);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, helper);

  const auto number = GetParam();
  for (auto i = 0; i < number; ++i)
  {
    // add a single file to it.
    helper->AddFile();
  }

  // give a little more than the timeout
  helper->Wait( static_cast<long long>(timeout * 2) );

  EXPECT_EQ(number, helper->Added());

  EXPECT_NO_THROW(::MonitorsManager::Stop(id));

  EXPECT_TRUE(Remove(id));
  delete helper;
}

INSTANTIATE_TEST_SUITE_P(
  MonitorsManagerAdd,
  ValidateNumberOfFilesAdded,
  ::testing::Values(
    0, 1, 42, 100
  ));