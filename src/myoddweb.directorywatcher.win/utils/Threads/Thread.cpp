#include "Thread.h"

#include "..\Wait.h"

namespace myoddweb::directorywatcher
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

  Thread::Thread(const TCallback& function) : Thread()
  {
    _localWorker = new LocalThreadRunner(function);
    CreateWorker(_localWorker);
  }

  Thread::Thread(Worker& runner) : Thread()
  {
    CreateWorker(&runner);
  }

  /**
   * \brief the destructor
   */
  Thread::~Thread()
  {
    Wait();
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
    _future = new std::future<void>(std::async(std::launch::async, &ThreadRunner::Run, runner));
#else
    _thread = new std::thread(&Thread::StaticWorker, worker);
#endif

    // save the parent worker.
    _parentWorker = worker;
  }

  /**
   * \brief wait a little bit for the thread to finish
   * \param timeout the number of ms we want to wait for the thread to complete.
   * \return either timeout of complete if the thread completed.
   */
  Thread::wait_result Thread::WaitFor(const long long timeout)
  {
    Wait::SpinUntil([&] 
      {
        return _parentWorker == nullptr || _parentWorker->Completed();
      },  
      timeout);

    return (_parentWorker == nullptr || _parentWorker->Completed()) ?
      wait_result::complete :
      wait_result::timeout;
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
      (*_thread).join();
    }
    delete _thread;
    _thread = nullptr;
#endif
  }

#ifndef MYODDWEB_USE_FUTURE
  /**
   * \brief static runner used by std::thread to call the runner.
   * \param worker the runner that we will be calling.
   */
  void Thread::StaticWorker(Worker* worker )
  {
    worker->Launch();
  }
#endif
}
