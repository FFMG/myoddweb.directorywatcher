// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    class Timer final : public threads::Worker
    {
      threads::TCallback _function;
      const long long _delayTimeMilliseconds;
      float _elapsedTimeMilliseconds;

    public:
      explicit Timer(threads::TCallback function, const long long delayTimeMilliseconds)
        :
        _function(std::move(function)),
        _delayTimeMilliseconds( delayTimeMilliseconds),
        _elapsedTimeMilliseconds(0)
      {
      }
            
      ~Timer()
      {
        Timer::Stop();
      }

      /**
       * \brief Give the worker a chance to do something in the loop
       *        Workers can do _all_ the work at once and simply return false
       *        or if they have a tight look they can return true until they need to come out.
       * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
       * \return true if we want to continue or false if we want to end the thread
       */
      bool OnWorkerUpdate( const float fElapsedTimeMilliseconds) override
      {
        if( MustStop() )
        {
          return false;
        }

        // check if we are ready.
        _elapsedTimeMilliseconds += fElapsedTimeMilliseconds;
        if( _elapsedTimeMilliseconds < _delayTimeMilliseconds)
        {
          return !MustStop();
        }

        //  restart the timer.
        _elapsedTimeMilliseconds = 0;

        // run the function
        _function();

        return !MustStop();
      }
    };
  }
}
