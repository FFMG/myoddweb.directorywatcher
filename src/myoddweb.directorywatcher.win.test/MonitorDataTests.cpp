#include "pch.h"

#include "../myoddweb.directorywatcher.win/monitors/MonitorData.h"

using myoddweb::directorywatcher::MonitorData;

TEST(MonitorData, BufferLenghValueIsSaved) {
  MonitorData md(100);
  ASSERT_EQ(100, md.BufferLength() );
}

TEST(MonitorData, BufferIsNullByDefault) {
  MonitorData md(100);
  ASSERT_EQ(nullptr, md.Buffer());
}

TEST(MonitorData, DirectoryHandleIsNullByDefault) {
  MonitorData md(100);
  ASSERT_EQ(nullptr, md.DirectoryHandle());
}

TEST(MonitorData, DirectoryHandleIsSaved) {
  MonitorData md(100);
  auto x = 10;
  md.DirectoryHandle( static_cast<void*>(&x));
  ASSERT_EQ(static_cast<void*>(&x), md.DirectoryHandle());
}

TEST(MonitorData, ClearResetsDirectoryHandle) {
  MonitorData md(100);
  auto x = 10;
  md.DirectoryHandle(static_cast<void*>(&x));
  ASSERT_EQ(static_cast<void*>(&x), md.DirectoryHandle());
  md.Clear();
  ASSERT_EQ(nullptr, md.DirectoryHandle());
}

TEST(MonitorData, ClearResetsOverlaped) {
  MonitorData md(100);
  auto x = 10;
  md.Prepare(&x);
  ASSERT_EQ(static_cast<void*>(&x), md.Overlapped()->hEvent );
  md.Clear();
  ASSERT_EQ(nullptr, md.Overlapped()->hEvent );
}

TEST(MonitorData, ClearDoesNotChanedBufferLengh) {
  MonitorData md(100);
  ASSERT_EQ(100, md.BufferLength());
  md.Clear();
  ASSERT_EQ(100, md.BufferLength());
}
