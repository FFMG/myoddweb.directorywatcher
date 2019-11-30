#pragma once
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
    class Wait final
    {
      struct Helper
      {
        Wait* parent;
        long long ms;
        std::function<bool()> predicate;
      };

      /**
       * \brief the conditional locl
       */
      std::mutex _mutex;

      /**
       * \brief the thread wait
       */
      std::condition_variable _conditionVariable;

    private:
      /**
       * \brief constructor, prevent direct construction
       */
      Wait() = default;

    public:
      // prevent copies.
      Wait(const Wait&) = delete;
      const Wait& operator=(const Wait&) = delete;

      /**
       * \brief the number of ms we want to wait for.
       */
      static void Delay(const long long milliseconds);

      /**
      * \brief the number of ms we want to wait for.
      * \param condition if this returns true we will return.
      * \param milliseconds the number of milliseconds we want to wait for
      *        if the timeout is reached, we will simply get out.
      * \return true if the condition fired, false if we timeout
      */
      static bool SpinUntil(std::function<bool()> condition, const long long milliseconds);

    private:
      /**
       * \brief the number of ms we want to wait for and/or check for a condition
       *        this function does not do validations.
       * \param condition if this returns true we will return.
       * \param milliseconds the number of milliseconds we want to wait for
       *        if the timeout is reached, we will simply get out.
       * \return true if the condition fired, false if we timeout
       */
      static bool SpinUntilInternal(std::function<bool()> condition, const long long milliseconds);

      /**
       * \brief wait for the future to complete, if it does not complete in time we will get out.
       *        note that even if the timeout if very large, we will not spin for ever.
       * \param milliseconds the max amount of time we are prepared to wait for.
       * \param future the future we will be waiting for.
       * \return true if the future completed or false if we timed out.
       */
      static bool SpinUntilFutureComplete(const long long milliseconds, std::future<void>& future);

      bool Awaiter(const long long milliseconds, std::function<bool()>& condition);

      static void _Awaiter(Helper&& helper, std::promise<bool>&& callResult);
    };
  }
}