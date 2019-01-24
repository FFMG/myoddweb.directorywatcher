#include "pch.h"

#include "../myoddweb.directorywatcher.win/monitors/win/Data.h"
#include "../myoddweb.directorywatcher.win/monitors/WinMonitor.h"

using myoddweb::directorywatcher::win::Data;
using myoddweb::directorywatcher::WinMonitor;
using myoddweb::directorywatcher::Request;

TEST(Data, BufferLenghValueIsSaved) {
  const Request r = { L"c:\\", true };
  WinMonitor wm( 1, r );
  Data md(wm, 100);
  ASSERT_EQ(100, md.BufferLength() );
}

TEST(Data, BufferIsNullByDefault) {
  const Request r = { L"c:\\", true };
  WinMonitor wm(1, r);
  Data md(wm, 100);
  ASSERT_EQ(nullptr, md.Buffer());
}

TEST(Data, DirectoryHandleIsNullByDefault) {
  const Request r = { L"c:\\", true };
  WinMonitor wm(1, r);
  Data md(wm, 100);
  ASSERT_EQ(nullptr, md.DirectoryHandle());
}
