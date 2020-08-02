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
long long WorkerId::NextId()
{
  static WorkerId workerId;
  return workerId.GetNextId();
}

/// <summary>
/// Get the next id.
/// </summary>
/// <returns></returns>
long long WorkerId::GetNextId()
{
  MYODDWEB_LOCK(_idLock);
  return _id++;
}
