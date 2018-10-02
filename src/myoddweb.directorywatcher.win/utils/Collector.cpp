//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
#include "Collector.h"
#include "Lock.h"

Collector::Collector()
{
}

Collector::~Collector()
{
}

/**
 * \brief Get the time now in milliseconds since 1970
 * \return the current ms time
 */
long long Collector::GetTimeMs() const
{
  const auto now = std::chrono::system_clock::now();
  const auto nowMs = std::chrono::time_point_cast<std::chrono::milliseconds>(now);

  auto value = nowMs.time_since_epoch();
  return value.count();
}

bool Collector::Add(const __int64 id, const EventAction action, const std::wstring& path, const std::wstring& file)
{
  // Create the event
  EventInformation e;
  e.id = id;
  e.action = action;
  e.name = path + file;
  e.timeMs = GetTimeMs();

  // lock
  auto guard = Lock(_lock);
  try
  {
    // add it.
    _events.push_back(e);

    // all good.
    return true;
  }
  catch(...)
  {
    // something wrong happened
    return false;
  }
}
