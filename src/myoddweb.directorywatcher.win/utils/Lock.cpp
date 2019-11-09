// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "Lock.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    // grab the lock
    Lock::Lock(std::recursive_mutex& lock) : _lock(lock)
    {
      _lock.lock();
    }

    // release the lock
    Lock::~Lock()
    {
      _lock.unlock();
    }
  }
}