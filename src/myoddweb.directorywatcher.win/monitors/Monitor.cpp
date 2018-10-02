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
#include "Monitor.h"

Monitor::Monitor( const __int64 id, const std::wstring& path, const bool recursive) : 
  _id( id ),
  _path( path ),
  _recursive(recursive )
{
  
}

Monitor::~Monitor()
{

}

/**
 * Get the id of the monitor
 * @return __int64 the id
 */
__int64 Monitor::Id() const
{
  return _id;
}

/**
 * Get the current path.
 * @return the path being checked.
 */
const std::wstring& Monitor::Path() const
{
  return _path;
}

/**
 * If this is a recursive monitor or not.
 * @return if recursive or not.
 */
bool Monitor::Recursive() const
{
  return _recursive;
}
