#include "Monitor.h"

Monitor::Monitor( const __int64 id ) : _id( id )
{
  
}

Monitor::~Monitor()
{

}

__int64 Monitor::Id() const
{
  return _id;
}