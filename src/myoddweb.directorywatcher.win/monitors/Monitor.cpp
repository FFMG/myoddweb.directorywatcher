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
