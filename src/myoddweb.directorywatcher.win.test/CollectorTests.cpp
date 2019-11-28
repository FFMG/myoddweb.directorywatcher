#include "pch.h"

#include "../myoddweb.directorywatcher.win/utils/Collector.h"
#include "../myoddweb.directorywatcher.win/utils/Event.h"
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
#include "../myoddweb.directorywatcher.win/utils/EventError.h"

using myoddweb::directorywatcher::Collector;
using myoddweb::directorywatcher::Event;
using myoddweb::directorywatcher::EventError;
using myoddweb::directorywatcher::EventAction;

TEST(Collector, EmptyCollectorReturnsNothing) {

  // create new one.
  Collector c;

  // empty
  std::vector<Event*> events;
  c.GetEvents(events);
  EXPECT_EQ( 0, events.size() );
}

TEST(Collector, PathIsValidWithTwoBackSlash) {

  // create new one.
  Collector c;
  c.Add( EventAction::Added, L"c:\\", L"\\foo\\bar.txt", true, EventError::None);

  // get it.
  std::vector<Event*> events;
  c.GetEvents(events);
  EXPECT_EQ(1, events.size() );

  EXPECT_TRUE(wcscmp(L"c:\\foo\\bar.txt", events[0]->Name) == 0);
  delete events[0];
}

TEST(Collector, PathIsValidWithOneBackSlashOnPath) {

  // create new one.
  Collector c;
  c.Add(EventAction::Added, L"c:\\", L"foo\\bar.txt", true, EventError::None);

  // get it.
  std::vector<Event*> events;
  c.GetEvents(events);
  EXPECT_EQ(1, events.size() );

  EXPECT_TRUE(wcscmp(L"c:\\foo\\bar.txt", events[0]->Name) == 0);
  delete events[0];
}

TEST(Collector, PathIsValidWithOneBackSlashOnFileName) {

  // create new one.
  Collector c;
  c.Add(EventAction::Added, L"c:", L"\\foo\\bar.txt", true, EventError::None );

  // get it.
  std::vector<Event*> events;
  c.GetEvents(events);
  EXPECT_EQ(1, events.size() );

  EXPECT_TRUE(wcscmp(L"c:\\foo\\bar.txt", events[0]->Name)==0);
  delete events[0];
}