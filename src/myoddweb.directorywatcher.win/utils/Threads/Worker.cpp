#include "Worker.h"
#include <thread>
#include "../../monitors/Base.h"
#include "../Wait.h"

namespace myoddweb::directorywatcher::threads
{
  Worker::Worker() :
    _started(false),
    _completed(false)
  {
    // set he current time point
    _timePoint1 = std::chrono::system_clock::now();
    _timePoint2 = _timePoint1;
  }

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
   * \brief Function called to run the thread.
   */
  void Worker::Launch()
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
        MYODDWEB_ALERTABLE_SLEEP(1);
        return Completed();
      },
      timeout))
    {
      return WaitResult::timeout;
    }
    
    // done
    return WaitResult::complete;
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
      return OnWorkerStart();
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

          // call the derived class.
          if (!OnWorkerUpdate(fElapsedTimeMilliseconds))
          {
            // we are done
            break;
          }

          // sleep a bit, we must be alertable so we can pass/receive messages.
          MYODDWEB_ALERTABLE_SLEEP(MYODDWEB_MIN_THREADPOOL_SLEEP);

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
