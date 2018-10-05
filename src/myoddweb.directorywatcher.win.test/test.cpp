#include "pch.h"

#include "..\myoddweb.directorywatcher.win\utils\Collector.h"
#include "..\myoddweb.directorywatcher.win\utils\Event.h"

using myoddweb::directorywatcher::Collector;
using myoddweb::directorywatcher::Event;

TEST(Collector, EmptyCollectorReturnsNothing) {

  // create new one.
  auto x = new Collector();

  // empty
  std::vector<Event> events;
  EXPECT_EQ( 0, x->GetEvents(events) );
  EXPECT_EQ( 0, events.size() );
  EXPECT_EQ(0, events.size());
}