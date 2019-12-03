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

    public:
      Timer() : 
        _mustStop( false )
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
        return true;
      }

      void Stop() 
      {
        _mustStop = true;

        // zero ms 
        const auto zeroMilliseconds = std::chrono::milliseconds(0);

        // wait for a could of ms
        Wait::SpinUntil([=] 
          {
            if (!_future.valid())
            {
              // the value is not even set.
              return true;
            }
            const auto status = _future.wait_for(zeroMilliseconds);
            return (status == std::future_status::ready);
          }, 10000);

      }
    };
  }
}
