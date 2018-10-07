#include "pch.h"

#include "..\myoddweb.directorywatcher.win\utils\Collector.h"
#include "..\myoddweb.directorywatcher.win\utils\Event.h"

using myoddweb::directorywatcher::Collector;
using myoddweb::directorywatcher::Event;

TEST(Collector, EmptyCollectorReturnsNothing) {

  // create new one.
  Collector c;

  // empty
  std::vector<Event> events;
  EXPECT_EQ( 0, c.GetEvents(events) );
  EXPECT_EQ( 0, events.size() );
  EXPECT_EQ( 0, events.size());
}

TEST(Collector, PathIsValidWithTwoBackSlash) {

  // create new one.
  Collector c;
  c.Add(EventAction::Added, L"c:\\", L"\\foo\\bar.txt", true );

  // get it.
  std::vector<Event> events;
  EXPECT_EQ(1, c.GetEvents(events));

  EXPECT_EQ( L"c:\\foo\\bar.txt", events[0].Path);
}

TEST(Collector, PathIsValidWithOneBackSlashOnPath) {

  // create new one.
  Collector c;
  c.Add(EventAction::Added, L"c:\\", L"foo\\bar.txt", true );

  // get it.
  std::vector<Event> events;
  EXPECT_EQ(1, c.GetEvents(events));

  EXPECT_EQ(L"c:\\foo\\bar.txt", events[0].Path);
}

TEST(Collector, PathIsValidWithOneBackSlashOnFileName) {

  // create new one.
  Collector c;
  c.Add(EventAction::Added, L"c:", L"\\foo\\bar.txt", true );

  // get it.
  std::vector<Event> events;
  EXPECT_EQ(1, c.GetEvents(events));

  EXPECT_EQ(L"c:\\foo\\bar.txt", events[0].Path);
}