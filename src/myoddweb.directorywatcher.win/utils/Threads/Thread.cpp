// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "Thread.h"

#include "../../monitors/Base.h"
#include "../Wait.h"
#include "CallbackWorker.h"
#include "WaitResult.h"

namespace myoddweb::directorywatcher::threads
{
  Thread::Thread() :
    _localWorker(nullptr),
    _parentWorker(nullptr),
#ifdef MYODDWEB_USE_FUTURE
    _future(nullptr),
#else
    _thread(nullptr)
#endif
  {

  }

  /**
   * \brief simple worker thread with a unique callback funtion
   * \param function the callback function we will call.
   */
  Thread::Thread(const TCallback& function) : Thread()
  {
    _localWorker = new CallbackWorker(function);
    CreateWorker(_localWorker);
  }

  /**
   * \brief create a thread with a worker.
   * \param worker the worker class that will do the actual work.
   */
  Thread::Thread(Worker& worker) : Thread()
  {
    CreateWorker(&worker);
  }

  /**
   * \brief the destructor
   */
  Thread::~Thread()
  {
    // wait for the thread to complete.
    Wait();

    // we do not clear the parent here
    // as we did not create it.

    // clean up our worker if we used it.
    delete _localWorker;
    _localWorker = nullptr;
  }

  /**
   * \brief create the runner either with future/thread
   * \param worker the runner that we want to start working with.
   */
  void Thread::CreateWorker(Worker* worker)
  {
#ifdef MYODDWEB_USE_FUTURE
    _future = new std::future<void>(std::async(std::launch::async, &Worker::Start, worker));
#else
    _thread = new std::thread(&Worker::Start, worker);
#endif

    // save the parent worker.
    _parentWorker = worker;
  }

  /**
   * \brief wait a little bit for the thread to finish
   * \param timeout the number of ms we want to wait for the thread to complete.
   * \return either timeout of complete if the thread completed.
   */
  WaitResult Thread::WaitFor(const long long timeout)
  {
    // wait for the worker to complete.
    const auto result = WaitFor(_parentWorker, timeout);

    // if we are complete then we can complete the thread.
    if( result == WaitResult::complete)
    {
      Wait();
    }
    return result;
  }

  /**
   * \brief wait a little bit for the thread to finish
   * \param worker the worker we are waiting for.
   * \param timeout the number of ms we want to wait for the thread to complete.
   * \return either timeout of complete if the thread completed.
   */
  WaitResult Thread::WaitFor(Worker* worker, const long long timeout)
  {
    if (worker == nullptr || worker->Completed())
    {
      return WaitResult::complete;
    }

    if (!Wait::SpinUntil([&]
      {
        MYODDWEB_YIELD();
        return worker->Completed();
      },
      timeout))
    {
      return WaitResult::timeout;
    }
    return WaitResult::complete;
  }

  /**
   * \brief wait for the thread to complete.
   */
  void Thread::Wait()
  {
#ifdef MYODDWEB_USE_FUTURE
    if (_future != nullptr)
    {
      (*_future).wait();
    }
    // the future should have ended now
    delete _future;
    _future = nullptr;
#else
    if (_thread != nullptr)
    {
      try
      {
        (*_thread).join();
      }
      catch (...)
      {
        
      }
    }
    delete _thread;
    _thread = nullptr;
#endif
  }
}
