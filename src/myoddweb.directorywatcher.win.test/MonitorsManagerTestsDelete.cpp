#pragma once

#include "pch.h"

#include "../myoddweb.directorywatcher.win/utils/MonitorsManager.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"

#include "MonitorsManagerTestHelper.h"

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

  auto count = 0;
  // monitor that folder.
  const auto request = ::Request(helper->Folder(), false, function, 0);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, helper);

  // add a single file to it.
  const auto file = helper->AddFile();

  // delete it
  ASSERT_TRUE( helper->RemoveFile(file) );

  helper->Wait(100);

  ASSERT_EQ(0, helper->Added(true));
  ASSERT_EQ(0, helper->Removed(true));

  ASSERT_NO_THROW(::MonitorsManager::Stop(id));

  ASSERT_TRUE(Remove(id));
  delete helper;
}

TEST_P(ValidateNumberOfItemDeleted, CallbackWhenFileIsDeleted) {
  const auto timeout = 50;
  // create the helper.
  auto helper = new MonitorsManagerTestHelper();

  const auto number = std::get<0>(GetParam());
  const auto recursive = std::get<1>(GetParam());

  auto count = 0;
  // monitor that folder.
  const auto request = ::Request(helper->Folder(), recursive, function, timeout);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, helper);

  auto files = std::vector<std::wstring>();
  for (auto i = 0; i < number; ++i)
  {
    // add a single file to it.
    files.push_back( helper->AddFile() );
  }

  // delete them all
  for each (auto file in files)
  {
    ASSERT_TRUE(helper->RemoveFile(file));
  }

  // give a little more than the timeout
  helper->Wait(static_cast<long long>(timeout * 2));

  ASSERT_EQ(number, helper->Removed(true));

  ASSERT_NO_THROW(::MonitorsManager::Stop(id));

  ASSERT_TRUE(Remove(id));
  delete helper;
}

TEST_P(ValidateNumberOfItemDeleted, CallbackWhenFolderIsDeleted) {
  const auto timeout = 50;
  // create the helper.
  auto helper = new MonitorsManagerTestHelper();

  const auto number = std::get<0>(GetParam());
  const auto recursive = std::get<1>(GetParam());

  auto count = 0;
  // monitor that folder.
  const auto request = ::Request(helper->Folder(), recursive, function, timeout);
  const auto id = ::MonitorsManager::Start(request);
  Add(id, helper);

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
  helper->Wait(static_cast<long long>(timeout * 2));

  ASSERT_EQ(number, helper->Removed(false));

  ASSERT_NO_THROW(::MonitorsManager::Stop(id));

  ASSERT_TRUE(Remove(id));
  delete helper;
}
