// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <chrono>
#include <future>
#include "Wait.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class Timer
    {
    private:
      bool _mustStop;
      std::future<void> _future;

      long long _delay;

    public:
      Timer() : 
        _mustStop( false ),
        _delay( -1 )
      {
      }
            
      ~Timer()
      {
        Stop();
      }

      template<typename Function>
      bool Start(Function function, const long long& delay)
      {
        Stop();
        _mustStop = false;
        _delay = delay;
        const auto sleep = std::chrono::milliseconds(delay);
        _future = std::async(std::launch::async, [=]()
          {
            for (;;)
            {
              if (_mustStop)
              {
                return;
              }
              std::this_thread::sleep_for( sleep );
              if (_mustStop)
              {
                return;
              }
              function();
            }
          });
        std::this_thread::yield();
        return true;
      }

      void Stop() 
      {
        _mustStop = true;
        if (_delay == -1)
        {
          return;
        }

        // zero ms 
        const auto delay = std::chrono::nanoseconds(1);

        // wait for a could of ms
        Wait::SpinUntil([=] 
          {
            if (!_future.valid())
            {
              // the value is not even set.
              return true;
            }
            const auto status = _future.wait_for(delay);
            return (status == std::future_status::ready);
          }, _delay );

        // we are done
        _delay = -1;
      }
    };
  }
}
