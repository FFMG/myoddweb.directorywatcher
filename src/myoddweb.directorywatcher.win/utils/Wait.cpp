// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "Wait.h"
#include <chrono>
#include <utility>
#include "../monitors/Base.h"
#include "Logger.h"
#include "LogLevel.h"

#if defined( _WIN32) || defined(_WIN64 )
  constexpr auto MYODDWEB_MAX_WAIT_INT = static_cast<unsigned int>(-1);
#else
  #include <limits> 
  constexpr auto MYODDWEB_MAX_WAIT_INT = std::numeric_limits<int>::max()
#endif

#if defined( _WIN32) || defined(_WIN64 )
  #include <windows.h>
#endif

namespace myoddweb:: directorywatcher
{
  /**
   * \brief initialise the value
   */
  std::atomic<int> Wait::_yieldCounter = std::atomic<int>(0);

  /**
    * \brief the number of ms we want to wait for.
    * \param milliseconds the number of milliseconds we want to wait for.
    */
  void Wait::Delay(const long long milliseconds)
  {
    // call the internal spin without any callback
    // so we _will_ timeout, no need for the return value.
    SpinUntilInternal(milliseconds);
  }

  /**
   * \brief the number of ms we want to wait for.
   * \param condition if this returns true we will return.
   * \param milliseconds the number of milliseconds we want to wait for
   *        if the timeout is reached, we will simply get out.
   * \return true if the condition fired, false if we timeout
   */
  bool Wait::SpinUntil( std::function<bool()> condition, const long long milliseconds)
  {
    return SpinUntilInternal( condition, milliseconds);
  }

  /**
    * \brief the number of ms we want to wait for and/or check for a condition
    *        this function does not do validations.
    * \param condition if this returns true we will return.
    * \param milliseconds the number of milliseconds we want to wait for
    *        if the timeout is reached, we will simply get out.
    * \return true if the condition fired, false if we timeout
    */
  bool Wait::SpinUntilInternal( std::function<bool()>& condition, const long long milliseconds)
  {
    Wait waiter;

    // call the awaiter.
    return waiter.Awaiter( std::move(condition), milliseconds );
  }

  /**
   * \brief the number of ms we want to spin for
   *        this function does not do validations.
   * \param milliseconds the number of milliseconds we want to wait for
   *        if the timeout is reached, we will simply get out.
   * \return if the code ran successfully or if there was an issue/error
   */
  bool Wait::SpinUntilInternal(const long long milliseconds)
  {
    Wait waiter;

    // start the thread with the arguments we have
    return waiter.Awaiter( nullptr, milliseconds );
  }

  /**
    * \brief wait for the future to complete, if it does not complete in time we will get out.
    *        note that even if the timeout if very large, we will not spin for ever.
    * \param milliseconds the max amount of time we are prepared to wait for.
    * \param future the future we will be waiting for.
    * \return true if the future completed or false if we timed out.
    */
  template <typename T>
  bool Wait::SpinUntilFutureComplete(std::future<T>& future, const long long milliseconds)
  {
    // when we consider this timed-out
    const auto until = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds);

    const auto waitFor = std::chrono::nanoseconds(1);
    auto concurentThreadsSupported = std::thread::hardware_concurrency();
    if( 0 == concurentThreadsSupported )
    {
      concurentThreadsSupported = 1;
    }
    for (auto count = 0; count < MYODDWEB_MAX_WAIT_INT; ++count)
    {
      if (future.valid())
      {
        // wait for that thread to complete.
        // it could hang forever as well
        // but we tried to make sure that it never does.
        // but it is posible that the condition() will hang.
        const auto status = future.wait_for(waitFor);
        if (status == std::future_status::ready)
        {
          // the thread is finished
          return true;
        }
      }

      if( count % concurentThreadsSupported != 0 )
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(count % concurentThreadsSupported));
      }
      else
      {
        std::this_thread::yield();
      }       

      // are we done?
      if (std::chrono::high_resolution_clock::now() >= until)
      {
        // we timed out.
        return false;
      }
    }

    // if we ever get here ... we reached the spin limit
    // this future is gone, it will never complete.
    return false;
  }

  /**
   * \brief wait for a Thread to complete, if it does not complete we will get out.
   *        note that even if the timeout if very large, we will not spin for ever.
   * \param milliseconds the max amount of time we are prepared to wait for.
   * \param thread the thread we will be waiting for.
   * \return true if the future completed or false if we timed out.
   */
  bool Wait::SpinUntilThreadComplete(threads::Thread& thread, const long long milliseconds)
  {
    // when we consider this timed-out
    const auto until = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds);

    const auto waitFor = 1/*ms*/;
    auto concurentThreadsSupported = std::thread::hardware_concurrency();
    if (0 == concurentThreadsSupported)
    {
      concurentThreadsSupported = 1;
    }
    for (unsigned int count = 0; count < MYODDWEB_MAX_WAIT_INT; ++count)
    {
      // wait for that thread to complete.
      // it could hang forever as well
      // but we tried to make sure that it never does.
      // but it is posible that the condition() will hang.
      const auto status = thread.WaitFor( waitFor );
      if (status == threads::WaitResult::complete )
      {
        // the thread is finished
        return true;
      }

      if (count % concurentThreadsSupported != 0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(count % concurentThreadsSupported));
      }
      else
      {
        std::this_thread::yield();
      }

      // are we done?
      if (std::chrono::high_resolution_clock::now() >= until)
      {
        // we timed out.
        return false;
      }
    }

    // if we ever get here ... we reached the spin limit
    // this future is gone, it will never complete.
    return false;
  }

  /**
   * \brief the main function that does all the waiting.
   *        the promise will contain the result of the waiting
   *          - false = we timed-out
   *          - true = the condition returned true and we stopped waiting.
   * \param condition the condition we wan to run to return out of the function
   *        if empty/null then we will never check the condition
   * \param milliseconds the maximum amount of time we want to wait
   *        if the condition is not set then we will always return false and wait for that number of ms.
   */
  bool Wait::Awaiter
  (
    std::function<bool()>&& condition, 
    const long long milliseconds
  )
  {
    auto result = false;
    std::unique_lock<std::mutex> lock(_mutex);
    try
    {
      // when we want to sleep until.
      const auto until = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds == -1 ? 1 : milliseconds);
      for (unsigned int count = 0; count < MYODDWEB_MAX_WAIT_INT; ++count)
      {
        YieldOnce();

        if (condition != nullptr)
        {
          try
          {
            if (condition())
            {
              result = true;
              break;
            }
          }
          catch (const std::exception& e)
          {
            // log the error
            Logger::Log(LogLevel::Panic, L"Caught exception '%hs' Awaiting on condition!", e.what());

            // this is bad ... the condition failed
            // we might as well get out as it will probably fail over and over again
            result = false;
            break;
          }
        }

        // are we done?
        if (milliseconds != -1 && std::chrono::high_resolution_clock::now() >= until)
        {
          break;
        }
      }
    }
    catch (const std::exception& e)
    {
      // log the error
      Logger::Log(LogLevel::Panic, L"Caught exception '%hs' Awaiting on condition!", e.what());
      result = false;
    }

    // release everything
    lock.unlock();
    _conditionVariable.notify_one();

    // all done
    return result;
  }

  void Wait::YieldOnce()
  {
    // one ms wait.
    static const auto oneMillisecond = std::chrono::milliseconds(1);
    static const auto zeroMilliseconds = std::chrono::milliseconds(0);

#if defined(_WIN32)
    switch (_yieldCounter)
    {
    case 1:
      ::SleepEx(0, true);
      // ::SleepEx(0, false);
      break;

    case 2:
      ::SleepEx(1, true);
      // ::SleepEx(1, false);
      break;

    case 3:
      // slee a little bit
      std::this_thread::sleep_for(oneMillisecond);
      break;

    case 4:
      // slee a little bit
      std::this_thread::sleep_for(zeroMilliseconds);
      break;

    case 5:
      // yield.
      std::this_thread::yield();
      break;

    case 6:
      ::YieldProcessor();
      break;

    case 7:
      ::SwitchToThread();
      break;

    default:
      _yieldCounter = 0;
      break;
    }
#endif
    // move forward.
    ++_yieldCounter;
  }
}