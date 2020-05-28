// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "Worker.h"
#include <thread>
#include "../../monitors/Base.h"
#include "../Wait.h"

namespace myoddweb::directorywatcher::threads
{
  Worker::Worker() :
    _started(false),
    _completed(false),
    _muststop( false )
  {
    // set he current time point
    _timePoint1 = std::chrono::system_clock::now();
    _timePoint2 = _timePoint1;
  }

  Worker::~Worker() = default;

  /**
   * \brief if the thread has completed or not.
   * \return if the thread is still running.
   */
  [[nodiscard]]
  bool Worker::Completed() const
  {
    return _completed;
  }

  /**
   * \brief If the worker has started or not.
   * \return if the worker is still running.
   */
  [[nodiscard]]
  bool Worker::Started() const
  {
    return _started;
  }

  /**
    * \brief If the worker has been told to stop or not.
    * \return if the worker must stop.
    */
  [[nodiscard]]
  bool Worker::MustStop() const
  {
    return _muststop;
  }

  /**
   * \brief non blocking call to instruct the thread to stop.
   */
  void Worker::Stop()
  {
    _muststop = true;
  }

  /**
   * \brief Function called to run the thread.
   */
  void Worker::Start()
  {
    // start the thread, if it returns false
    // then we will get out.
    if (!WorkerStart())
    {
      _completed = true;
      return;
    }

    // run the code
    WorkerRun();

    // the thread has ended.
    WorkerEnd();
  }

  /**
   * \brief stop the running thread and wait
   * \param timeout how long we want to wait
   * \return if the worker completed or if we timeed out.
   */
  WaitResult Worker::StopAndWait(const long long timeout)
  {
    try
    {

      // then wait for it to complete.
      if (!Started() || Completed())
      {
        // stopped already or not ever running
        return WaitResult::complete;
      }

      // stop it
      Stop();

      // wait for it
      if (false == Wait::SpinUntil([&]
        {
          return Completed();
        },
        timeout))
      {
        return WaitResult::timeout;
      }

      // done
      return WaitResult::complete;
    }
    catch( ... )
    {
      return WaitResult::timeout;
    }
  }

  /**
   * \brief called when the thread is starting
   *        this should not block anything
   */
  bool Worker::WorkerStart()
  {
    // the thread is not completed.
    _completed = false;

    // the thread has started work.
    // we could argue that this flag should be set
    // after `OnWorkerStart()` but this is technically all part of the same thread.
    _started = true;
    try
    {
      if (!OnWorkerStart())
      {
        // because the worker failed to start this is now 'completed'
        _completed = true;
        return false;
      }
      return true;
    }
    catch (const std::exception& e)
    {
      _exceptions.push_back(e);
      return false;
    }
    catch ( ... )
    {
      return false;
    }
  }

  /**
   * \brief the main body of the thread runner
   *        this function will run until the end of the thread.
   */
  void Worker::WorkerRun()
  {
    try
    {
      auto concurentThreadsSupported = std::thread::hardware_concurrency();
      if (0 == concurentThreadsSupported)
      {
        concurentThreadsSupported = 1;
      }

      // the amount of time we want to put our thread to sleep
      // so we do not cause a tight loop to burn the CPU.
      const auto threadSleep = std::chrono::milliseconds(MYODDWEB_MIN_THREADPOOL_SLEEP);

      auto count = 0;
      for (;;)
      {
        try
        {
          // update and calculate the elapsed time.
          _timePoint2 = std::chrono::system_clock::now();
          const std::chrono::duration<float, std::milli> elapsedTime = _timePoint2 - _timePoint1;
          _timePoint1 = _timePoint2;
          const auto fElapsedTimeMilliseconds = elapsedTime.count();

          // were we told to stop?
          if( _muststop )
          {
            break;
          }

          // call the derived class.
          if (!OnWorkerUpdate(fElapsedTimeMilliseconds))
          {
            // we are done
            break;
          }

          // sleep a bit, we must be alertable so we can pass/receive messages.
          MYODDWEB_YIELD();

          // we now need to slow the thread down a little more
          if (count % concurentThreadsSupported != 0)
          {
            std::this_thread::sleep_for(threadSleep);
            ++count;
          }
          else
          {
            std::this_thread::yield();
            count = 1;
          }
        }
        catch( ... )
        {
          // log it?
        }
      }
    }
    catch (const std::exception& e)
    {
      _exceptions.push_back(e);
    }
    catch ( ... )
    {
      // log it
    }
  }

  /**
   * \brief called when the thread is ending
       *        this should not block anything
   */
  void Worker::WorkerEnd()
  {
    try
    {
      OnWorkerEnd();
    }
    catch (const std::exception& e)
    {
      _exceptions.push_back( e );
    }
    catch ( ... )
    {
      // log it
    }

    // whatever happens, we have now completed
    _completed = true;
  }
}
