﻿#include "pch.h"

#include "../myoddweb.directorywatcher.win/monitors/win/Data.h"
#include "../myoddweb.directorywatcher.win/monitors/WinMonitor.h"

using myoddweb::directorywatcher::win::Data;
using myoddweb::directorywatcher::WinMonitor;
using myoddweb::directorywatcher::Request;

TEST(Data, BufferLenghValueIsSaved) {
  const auto r = Request( L"c:\\", true, nullptr, 0);
  const WinMonitor wm( 1, 2, r );
  auto fnc = Data::DataCallbackFunction();
  const Data md(wm, 
    FILE_NOTIFY_CHANGE_FILE_NAME,
    false,
    fnc,
    100);
  ASSERT_EQ(100, md.BufferLength() );
}

TEST(Data, DirectoryHandleIsNullByDefault) {
  const auto r = Request(L"c:\\", true, nullptr, 0);
  const WinMonitor wm(1, 2, r);
  auto fnc = Data::DataCallbackFunction();
  const Data md(wm, FILE_NOTIFY_CHANGE_FILE_NAME, false, fnc, 100);
  ASSERT_EQ(nullptr, md.DirectoryHandle());
}
