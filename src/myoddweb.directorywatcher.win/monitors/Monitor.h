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
#include "../utils/Collector.h"

class Monitor
{
public:
  Monitor( __int64 id, const std::wstring& path, bool recursive );
  virtual ~Monitor();

  __int64 Id() const;
  const std::wstring& Path() const;
  bool Recursive() const;
  Collector& EventsCollector() const;

public:
  virtual bool Start() = 0;
  virtual void Stop() = 0;

protected:
  void AddEvent(EventAction action, const std::wstring& fileName) const;

private:
  const __int64 _id;
  const std::wstring _path;
  const bool _recursive;

  Collector* _eventCollector;
};