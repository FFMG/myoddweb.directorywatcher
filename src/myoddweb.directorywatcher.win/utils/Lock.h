// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <mutex>

namespace myoddweb
{
  namespace directorywatcher
  {
#ifdef _DEBUG
    class Lock
#else
    class Lock final
#endif
    {
    public:
      explicit Lock(std::recursive_mutex& lock);
      virtual ~Lock();

    private:
      std::recursive_mutex& _lock;
    };
  }
}

#ifdef _DEBUG
/**
  * \brief turn full Lock debuging on, will output the where the lock is obtained/released
  *        this has a fairly big performance impact given the number of locks we obtain.
  */
#define MYODDWEB_DEBUG_LOG 0
#else
#define MYODDWEB_DEBUG_LOG 0
#endif // DEBUG
  
#if MYODDWEB_DEBUG_LOG
  #include <utility>
  #include "windows.h"
  #include <debugapi.h>
  namespace myoddweb
  {
    namespace directorywatcher
    {
      class LockDebug final : private Lock
      {
      public:
        explicit LockDebug(std::recursive_mutex& lock, std::string functionSig ) :
         Lock( lock ),
        _functionSig(std::move(functionSig))
        {
          LogStart();
        }
        virtual ~LockDebug()
        {
          LogEnd();
        }

      private:
        std::string _functionSig;
        void LogStart() const
        {
          std::string o = "Take Lock: ";
          o += _functionSig;
          o += "\n";
          OutputDebugStringA(o.c_str());
        }
        void LogEnd() const
        {
          std::string o = "Leave Lock: ";
          o += _functionSig;
          o += "\n";
          OutputDebugStringA(o.c_str());
        }
      };
    }
  }
  #define MYODDWEB_LOCK(mutex) LockDebug g##__LINE__(mutex, __FUNCSIG__ );
#else
  #define MYODDWEB_LOCK(mutex) Lock g##__LINE__(mutex);
#endif 