#include "pch.h"

#include "../myoddweb.directorywatcher.win/monitors/win/Data.h"
#include "../myoddweb.directorywatcher.win/monitors/WinMonitor.h"

using myoddweb::directorywatcher::win::Data;
using myoddweb::directorywatcher::WinMonitor;
using myoddweb::directorywatcher::Request;

TEST(Data, BufferLenghValueIsSaved) {
  auto r = Request( L"c:\\", true, nullptr, 0);
  WinMonitor wm( 1, r );
  Data md(wm, 100);
  ASSERT_EQ(100, md.BufferLength() );
}

TEST(Data, BufferIsNullByDefault) {
  auto r = Request(L"c:\\", true, nullptr, 0);
  WinMonitor wm(1, r);
  Data md(wm, 100);
  ASSERT_EQ(nullptr, md.Buffer());
}

TEST(Data, DirectoryHandleIsNullByDefault) {
  auto r = Request(L"c:\\", true, nullptr, 0);
  WinMonitor wm(1, r);
  Data md(wm, 100);
  ASSERT_EQ(nullptr, md.DirectoryHandle());
}
