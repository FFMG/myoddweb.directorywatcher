// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <chrono>
#include <thread>

namespace myoddweb
{
  namespace directorywatcher
  {
    class Timer
    {
    private:
      bool _mustStop;
      std::thread* _thread;

    public:
      Timer() : 
        _mustStop( false ),
        _thread(nullptr)
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
        _thread = new std::thread([=]()
          {
            for (;;)
            {
              if (_mustStop)
              {
                return;
              }
              std::this_thread::sleep_for(
                std::chrono::milliseconds( delay )
              );
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
        if (_thread != nullptr && _thread->joinable() )
        {
          // we can detach ourselves from the timer
          // we asked for it to stop and we are hoping that it
          // will indeed stop, but we cannot control if it is locked 
          // inside a calling function.
          _thread->detach();
        }

        delete _thread;
        _thread = nullptr;
      }
    };
  }
}
