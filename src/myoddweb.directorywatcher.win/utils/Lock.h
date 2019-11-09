// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

#include <mutex>

namespace myoddweb
{
  namespace directorywatcher
  {
    class Lock final
    {
    public:
      explicit Lock(std::recursive_mutex& lock);
      ~Lock();

    private:
      std::recursive_mutex& _lock;
    };
  }
}
  