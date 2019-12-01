#pragma once
#include "Wait.h"
#include <chrono>
#include <thread>
#include <condition_variable>
#include <functional>
#include <future>
#include <limits> 

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
      * \brief the number of ms we want to wait for.
      * \param milliseconds the number of milliseconds we want to wait for.
      */
    void Wait::Delay(const long long milliseconds)
    {
      // call the internal spin without any callback
      // so we _will_ timeout, no need for the return value.
      SpinUntilInternal(nullptr, milliseconds);
    }

    /**
     * \brief the number of ms we want to wait for.
     * \param condition if this returns true we will return.
     * \param milliseconds the number of milliseconds we want to wait for
     *        if the timeout is reached, we will simply get out.
     * \return true if the condition fired, false if we timeout
     */
    bool Wait::SpinUntil(std::function<bool()> condition, const long long milliseconds)
    {
      return SpinUntilInternal(condition, milliseconds);
    }

    /**
      * \brief the number of ms we want to wait for and/or check for a condition
      *        this function does not do validations.
      * \param condition if this returns true we will return.
      * \param milliseconds the number of milliseconds we want to wait for
      *        if the timeout is reached, we will simply get out.
      * \return true if the condition fired, false if we timeout
      */
    bool Wait::SpinUntilInternal(std::function<bool()> condition, const long long milliseconds)
    {
      Wait waiter;
      std::promise<bool> callResult;
      auto callFuture = callResult.get_future();

      // start the thread with the arguments we have
      auto future = std::async(std::launch::async,
        &Awaiter, 
        &waiter, 
        std::move(condition),
        milliseconds, 
        std::move(callResult)
      );

      // wait for the future to complete ... or timeout.
      // if we return false, we gave up waiting.
      if (!SpinUntilFutureComplete(milliseconds, future))
      {
        // we timed out, so something failed...
        return false;
      }

      // our thread competed, get out.
      return callFuture.get();
    }

    /**
      * \brief wait for the future to complete, if it does not complete in time we will get out.
      *        note that even if the timeout if very large, we will not spin for ever.
      * \param milliseconds the max amount of time we are prepared to wait for.
      * \param future the future we will be waiting for.
      * \return true if the future completed or false if we timed out.
      */
    bool Wait::SpinUntilFutureComplete(const long long milliseconds, std::future<void>& future)
    {
      // how long we will allow it to run for
      // note that we are adding a little bit of padding.
      const int padding = 10;
      const auto until = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds + padding);

      const auto oneMillisecond = std::chrono::milliseconds(1);
      for (auto count = 0; count < std::numeric_limits<int>::max(); ++count)
      {
        // wait for that thread to complete.
        // it could hang forever as well
        // but we tried to make sure that it never does.
        // but it is posible that the condition() will hang.
        auto status = future.wait_for(oneMillisecond);
        if (status == std::future_status::ready)
        {
          // the thread is finished
          return true;
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
     *          - false = we timedout
     *          - true = the condition returned true and we stopped waiting.
     * \param condition the condition we wan to run to return out of the function
     *        if empty/null then we will never check the condition
     * \param milliseconds the maximum amount of time we want to wait
     *        if the condition is not set then we will always return false and wait for that number of ms.
     * \param promise the promise that we will use to set the result.
     */
    void Wait::Awaiter
    (
      std::function<bool()>&& condition, 
      const long long milliseconds, 
      std::promise<bool>&& callResult
    )
    {
      bool result = false;
      std::unique_lock<std::mutex> lock(_mutex);
      try
      {
        if (condition == nullptr)
        {
          // just wait for 'x' ms.
          _conditionVariable.wait_for(lock, std::chrono::milliseconds(milliseconds));

          // simple timeout, always false
          // because the condition could never be 'true'
          result = true;
        }
        else
        {
          // one ms wait.
          const auto oneMillisecond = std::chrono::milliseconds(1);
          const auto zeroMilliseconds = std::chrono::milliseconds(0);

          // when we want to sleep until.
          const auto until = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds);
          for (auto count = 0; count < std::numeric_limits<int>::max(); ++count)
          {
            try
            {
              if (condition())
              {
                result = true;
                break;
              }
            }
            catch (...)
            {
              // this is bad ... the condition failed.
            }

            if (count % 4 == 0)
            {
              // slee a little bit
              std::this_thread::sleep_for(oneMillisecond);
            }
            else if (count % 2 == 0)
            {
              // slee a little bit
              std::this_thread::sleep_for(zeroMilliseconds);
            }
            else
            {
              // yield.
              std::this_thread::yield();
            }

            // are we done?
            if (std::chrono::high_resolution_clock::now() >= until)
            {
              break;
            }
          }
        }
      }
      catch (...)
      {
        //  something broke ... maybe we should re-throw.
        result = false;
      }

      // release everything
      lock.unlock();
      _conditionVariable.notify_one();

      // all done
      callResult.set_value( result );
    }
  }
}