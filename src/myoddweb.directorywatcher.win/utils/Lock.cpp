#include "Lock.h"

// grab the lock
Lock::Lock(std::recursive_mutex& lock) : _lock( lock )
{
  _lock.lock();
}

// release the lock
Lock::~Lock()
{
  _lock.unlock();
}
