#include "WorkerId.h"
#include "../Lock.h"

WorkerId::WorkerId() :
  _id(1)  //  we do not wan the number to be zero because
          //  that id is used as the 'debug' number
{
}

/// <summary>
/// Get the next available id.
/// </summary>
/// <returns></returns>
long long WorkerId::next_id()
{
  static WorkerId workerId;
  return workerId.get_next_id();
}

/// <summary>
/// Get the next id.
/// </summary>
/// <returns></returns>
long long WorkerId::get_next_id()
{
  MYODDWEB_LOCK(_idLock);
  return _id++;
}
