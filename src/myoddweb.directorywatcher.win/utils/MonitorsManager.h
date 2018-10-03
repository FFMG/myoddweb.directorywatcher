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
#include <mutex>
#include <unordered_map>
#include "../monitors/Monitor.h"

class MonitorsManager
{
protected:
  MonitorsManager();
  virtual ~MonitorsManager();

public:
  static __int64 Start(const wchar_t* path, bool recursive);
  static bool Stop(__int64 id );

protected:
  Monitor* CreateAndStart(const std::wstring& path, bool recursive);
  bool Remove( __int64 id );
  static __int64 GetId();
protected:
  // The file lock
  static std::recursive_mutex _lock;

  // the singleton
  static MonitorsManager* _instance;
  static MonitorsManager* Instance();

protected:
  typedef std::unordered_map<__int64, Monitor*> MonitorMap;
  MonitorMap _monitors;
};
