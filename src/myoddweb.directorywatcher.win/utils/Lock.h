// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "../monitors/Base.h"

namespace myoddweb
{
  namespace directorywatcher
  {
#if MYODDWEB_DEBUG_LOG
    class Lock
#else
    class Lock final
#endif
    {
    public:
      explicit Lock(MYODDWEB_MUTEX& lock);

#if MYODDWEB_DEBUG_LOG
      virtual ~Lock();
#else
      ~Lock(); 
#endif

      Lock() = delete;
      Lock(const Lock&) = delete;
      Lock(Lock&&) = delete;
      const Lock& operator=(const Lock&) = delete;
      Lock& operator=(const Lock&&) = delete;

    private:
      MYODDWEB_MUTEX& _lock;
    };
  }
}

#ifdef _DEBUG
/**
  * \brief turn full Lock debuging on, will output the where the lock is obtained/released
  *        this has a fairly big performance impact given the number of locks we obtain.
  *        1 -is looking for deadlock
  *        2- is full logging
  */
#define MYODDWEB_DEBUG_LOG 0
#else
#define MYODDWEB_DEBUG_LOG 0
#endif // DEBUG
  
#if MYODDWEB_DEBUG_LOG
  #if !defined(_DEBUG)
    #error "You cannot use debug log in release mode!"
  #endif

  #include "../monitors/Base.h"
  #include <utility>
  #include "windows.h"
  #include <debugapi.h>
  #include <chrono>
  #include <sstream>
  namespace myoddweb
  {
    namespace directorywatcher
    {
      class LockTry
      {
      public:
        explicit LockTry(MYODDWEB_MUTEX& lock, std::string functionSig) :
          _functionSig(std::move(functionSig)),
          _lock( lock )
        {
          const long long slowLock = 500;
          const auto start = std::chrono::high_resolution_clock::now();
          auto check = slowLock;
          for (;;)
          {
            if (_lock.try_lock())
            {
              const auto stop = std::chrono::high_resolution_clock::now();
              const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
              const auto d = duration.count();
              if (d > slowLock)
              {
                std::stringstream ss;
                ss << "Time taken to get lock: " << duration.count() << " milliseconds for " << _functionSig << "\n";
                MYODDWEB_OUT(ss.str().c_str());
              }
              // we got he lock
              break;
            }

            const auto stop = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            const long long d = duration.count();
            if (d > check)
            {
              check += slowLock; // next check
              std::stringstream ss;
              ss << "Lock is slow: " << duration.count() << " milliseconds for " << _functionSig  << "\n";
              MYODDWEB_OUT(ss.str().c_str());
            }
          }
        }
        virtual ~LockTry()
        {
          _lock.unlock();
        }

      protected:
        std::string _functionSig;
        MYODDWEB_MUTEX& _lock;
      };

      class LockDebug final : public LockTry
      {
      public:
        explicit LockDebug(MYODDWEB_MUTEX& lock, std::string functionSig) :
          LockTry(lock, std::move(functionSig))
        {
          LogStart();
        }
        virtual ~LockDebug()
        {
          LogEnd();
        }

      protected:
        void LogStart() const
        {
          const auto o = "Lock In: " + _functionSig +"\n";
          MYODDWEB_OUT(o.c_str());
        }
        void LogEnd() const
        {
          const auto o = "Lock Out: " + _functionSig + "\n";
          MYODDWEB_OUT(o.c_str());
        }
      };
    }
  }

  // 1- looking for deadlock
  // 2- full log
  #if MYODDWEB_DEBUG_LOG == 1 
    #define MYODDWEB_LOCK(mutex) LockTry MYODDWEB_DEC(__LINE__)(mutex, __FUNCSIG__);
  #elif MYODDWEB_DEBUG_LOG == 2
    #define MYODDWEB_LOCK(mutex)                                      \
    {                                                                 \
      const auto o = "Lock Wait: " + std::string(__FUNCSIG__) + "\n"; \
      MYODDWEB_OUT(o.c_str());                                        \
    }                                                                 \
    LockDebug MYODDWEB_DEC(__LINE__)(mutex, __FUNCSIG__ );
  #endif
#else
  #define MYODDWEB_LOCK(mutex) Lock MYODDWEB_DEC(__LINE__)(mutex);
#endif 