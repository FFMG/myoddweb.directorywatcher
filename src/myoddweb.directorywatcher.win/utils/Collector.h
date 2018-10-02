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
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "EventInformation.h"

/**
 * \brief Class that contains and manages all the events.
 */
class Collector
{
public:
  Collector();
  virtual ~Collector();

  bool Add(__int64 id, EventAction action, const std::wstring& path, const std::wstring& file);

private:
  /**
   * \brief the locks so we can add data.
   */
  std::recursive_mutex _lock;

  /**
   * \brief the events list
   */
  typedef std::vector<EventInformation> Events;
  Events _events;

  long long GetTimeMs() const;
  static std::wstring PathCombine(const std::wstring& lhs, const std::wstring& rhs);
};

