#pragma once
#include <thread>
#include <functional>

namespace myoddweb
{
  namespace directorywatcher
  {
    typedef std::function<void()> TCallback;

    class ThreadRunner
    {
      bool _started;
      bool _completed;
      bool _faulted;
      std::exception _exception;

    public:
      ThreadRunner(const ThreadRunner&) = delete;
      ThreadRunner(ThreadRunner&&) = delete;
      ThreadRunner& operator=(ThreadRunner&&) = delete;
      ThreadRunner& operator=(const ThreadRunner&) = delete;

      explicit ThreadRunner() :
        _started( false ),
        _completed( false ),
        _faulted(false)
      {
        
      }

      virtual ~ThreadRunner() = default;

      /**
       * \brief if the thread has completed or not.
       * \return if the thread is still running.
       */
      bool Completed() const
      {
        return _completed;
      }

      /**
       * Function called when we are running the thread.
       */
      virtual void Run()
      {
        _started = true;
        try
        {
          OnRunThread();
        }
        catch ( const std::exception& e)
        {
          _exception = e;
          _faulted = true;
        }
        _completed = true;
      }

    protected:
      virtual void OnRunThread() = 0;
    };

    class Thread final
    {
    public:
      enum class wait_result { // names for timed wait function returns
        complete,
        timeout,
      };
    private:
      class LocalThreadRunner final : public ThreadRunner
      {
        TCallback _function;
      public:
        explicit LocalThreadRunner(const TCallback& function)
        {
          _function = function;
        }

      protected:
        void OnRunThread() override
        {
          _function();
        }
      };

      ThreadRunner* _localRunner;
      ThreadRunner* _parentRunner;

#ifdef MYODDWEB_USE_FUTURE
      std::future<void>* _future;
#else
      std::thread* _thread;
      static void StaticRunner(ThreadRunner*);
#endif
      void CreateThread(ThreadRunner* runner );

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
      explicit Thread(ThreadRunner& runner);
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
