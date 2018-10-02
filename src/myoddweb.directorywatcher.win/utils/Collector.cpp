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

/**
 * \brief Combine 2 parts of a file into a single filename.
 *        we do not validate the path or even if the values make no sense.
 * \param lhs the left hand side
 * \param rhs the right hand side.
 * \return The 2 part connected togeter
 */
std::wstring Collector::PathCombine(const std::wstring& lhs, const std::wstring& rhs)
{
  // sanity check, if the lhs.length is 0, then we just return the rhs.
  const auto s = lhs.length();
  if( s == 0 )
  {
    return rhs;
  }

  // the two type of separators.
  const auto sep1 = L'/';
  const auto sep2 = L'\\';

  const auto l = lhs[s-1];
  if (l != sep1 && l != sep2) 
  {
#ifdef WIN32
    return lhs + sep2 + rhs;
#else
    return lhs + sep1 + rhs;
#endif
  }
  return lhs + rhs;
}

bool Collector::Add(const __int64 id, const EventAction action, const std::wstring& path, const std::wstring& file)
{
  try
  {
    // We first create the event outside the lock
    // that way, we only have the lock for the shortest
    // posible amount of time.
    EventInformation e;
    e.id = id;
    e.action = action;
    e.name = PathCombine(path, file);
    e.timeMs = GetTimeMs();

    wprintf(L"%s\n", e.name.c_str());

    // we can now get the lock so we can add data.
    // the lock is released automatically.
    auto guard = Lock(_lock);

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
