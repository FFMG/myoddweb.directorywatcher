#pragma once
#include <thread>
#include <functional>
#include "Worker.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    typedef std::function<void()> TCallback;

    class Thread final
    {
    public:
      enum class wait_result { // names for timed wait function returns
        complete,
        timeout,
      };
    private:
      class LocalThreadRunner final : public Worker
      {
        TCallback _function;
      public:
        explicit LocalThreadRunner(const TCallback& function)
        {
          _function = function;
        }

      protected:
        bool OnWorkerUpdate(float elapsedTime) override
        {
          _function();

          // we are done.
          return false;
        }
      };

      Worker* _localWorker;
      Worker* _parentWorker;

#ifdef MYODDWEB_USE_FUTURE
      std::future<void>* _future;
#else
      std::thread* _thread;
      static void StaticWorker(Worker*);
#endif
      void CreateWorker(Worker* worker );

      /**
       * \brief the common constructor, private as used to set default values.
       */
      Thread();

    public:
      Thread(const Thread&) = delete;
      Thread(Thread&&) = delete;
      Thread& operator=(Thread&&) = delete;
      Thread& operator=(const Thread&) = delete;

      explicit Thread(const TCallback& function );
      explicit Thread(Worker& worker);
      ~Thread();
      void Wait();

      /**
       * \brief wait a little bit for the thread to finish
       * \param timeout the number of ms we want to wait for the thread to complete.
       * \return either timeout of complete if the thread completed.
       */
      wait_result WaitFor( long long timeout );
    };
  }
}
