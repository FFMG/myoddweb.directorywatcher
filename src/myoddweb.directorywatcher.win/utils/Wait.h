#pragma once
#include <condition_variable>
#include <functional>
#include <future>

#include "Thread.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    class Wait final
    {
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
      static void Delay( long long milliseconds );

      /**
      * \brief the number of ms we want to wait for.
      * \param condition if this returns true we will return.
      * \param milliseconds the number of milliseconds we want to wait for
      *        if the timeout is reached, we will simply get out.
      * \return true if the condition fired, false if we timeout
      */
      static bool SpinUntil(std::function<bool()> condition, long long milliseconds);

      template <typename T>
      static bool SpinUntil( std::future<T>& future, long long milliseconds)
      {
        return SpinUntilFutureComplete(future, milliseconds);
      }

      static bool SpinUntil(Thread& thread, long long milliseconds)
      {
        return SpinUntilThreadComplete(thread, milliseconds);
      }

    private:
      /**
       * \brief the conditional lock
       */
      std::mutex _mutex;

      /**
       * \brief the thread wait
       */
      std::condition_variable _conditionVariable;

      /**
       * \brief the number of ms we want to wait for and/or check for a condition
       *        this function does not do validations.
       * \param condition if this returns true we will return.
       * \param milliseconds the number of milliseconds we want to wait for
       *        if the timeout is reached, we will simply get out.
       * \return true if the condition fired, false if we timeout
       */
      static bool SpinUntilInternal(std::function<bool()>& condition, long long milliseconds);

      /**
       * \brief the number of ms we want to spin for
       *        this function does not do validations.
       * \param milliseconds the number of milliseconds we want to wait for
       *        if the timeout is reached, we will simply get out.
       * \return if the code ran successfully or if there was an issue/error
       */
      static bool SpinUntilInternal( long long milliseconds);

      /**
       * \brief wait for the future to complete, if it does not complete in time we will get out.
       *        note that even if the timeout if very large, we will not spin for ever.
       * \param milliseconds the max amount of time we are prepared to wait for.
       * \param future the future we will be waiting for.
       * \return true if the future completed or false if we timed out.
       */
      template <typename T>
      static bool SpinUntilFutureComplete(std::future<T>& future, long long milliseconds );

      /**
       * \brief wait for a Thread to complete, if it does not complete we will get out.
       *        note that even if the timeout if very large, we will not spin for ever.
       * \param milliseconds the max amount of time we are prepared to wait for.
       * \param thread the thread we will be waiting for.
       * \return true if the future completed or false if we timed out.
       */
      static bool SpinUntilThreadComplete(Thread& thread, long long milliseconds);

      /**
       * \brief the main function that does all the waiting.
       *        the promise will contain the result of the waiting
       *          - false = we timedout 
       *          - true = the condition returned true and we stopped waiting.
       * \param condition the condition we wan to run to return out of the function
       *        if empty/null then we will never check the condition
       * \param milliseconds the maximum amount of time we want to wait
       *        if the condition is not set then we will always return false and wait for that number of ms.
       */
      bool Awaiter( std::function<bool()>&& condition, const long long milliseconds );
    };
  }
}
