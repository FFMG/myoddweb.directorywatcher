// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "WorkerPool.h"
#include "../../monitors/Base.h"
#include "../Lock.h"
#include "../Wait.h"
#include <execution>
#include "../Instrumentor.h"
#include "../Logger.h"
#include "../LogLevel.h"

namespace myoddweb :: directorywatcher :: threads
{
  WorkerPool::WorkerPool( const long long throttleElapsedTimeMilliseconds) :
    Worker(),
    _thread(nullptr),
    _throttleElapsedTimeMilliseconds(throttleElapsedTimeMilliseconds)
  {
  }

  WorkerPool::~WorkerPool()
  {
    Worker::CompleteAllOperations();

    // wait for the thread to complete.
    if (_thread != nullptr)
    {
      // we are about to delete the thread
      // so we must wait for the work to complete.
      // if there is a deadlock then we will wait forever...
      _thread->Wait();
    }

    delete _thread;
    _thread = nullptr;
  }

  #pragma region public functions
  /**
   * \brief stop multiple workers and wait
   * \param workers the workers we are waiting for.
   * \param timeout the number of ms we want to wait for them.
   * \return the result of the wait
   */
  WaitResult WorkerPool::StopAndWait(const std::vector<Worker*>& workers, long long timeout)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // start whatever needs to start
      ProcessThreadsAndWorkersWaiting();

      // then go around trying to lock th
      MYODDWEB_MUTEX lock;
      auto status = WaitResult::complete;
      // send an update for all of them at the same time
      std::for_each(
        std::execution::par,
        workers.begin(),
        workers.end(),
        [this,timeout,&lock,&status]( Worker* worker )
        {
          const auto result = StopAndWait(*worker, timeout );
          if( WaitResult::complete !=  result )
          {
            MYODDWEB_LOCK(lock);
            status = result;
          }
        }
      );

      // all done
      return status;
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
      return WaitResult::timeout;
    }
  }

  /**
   * \brief stop one of the worker and wait
   * \param worker the worker we are waiting for.
   * \param timeout the number of ms we want to wait.
   */
  WaitResult WorkerPool::StopAndWait(Worker& worker, const long long timeout)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // start whatever needs to start
      ProcessThreadsAndWorkersWaiting();

      // assune that the work is completed
      auto result = WaitResult::complete;
      if (!worker.Completed())
      {
        // if not wait for it to complete
        result = worker.StopAndWait(timeout);
      }

      // no need to remove it from our list of workers
      // it will be picked up by the next Update( ... ) call
      //
      return result;
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
      return WaitResult::timeout;
    }
  }

  /**
   * \brief add a worker to our worers pool.
   * \param worker the worker we are trying to add.
   */
  void WorkerPool::Add(Worker& worker)
  {
    Add({ &worker });
  }

  /**
   * \brief add multiple workers at once
   * \param workers the workers we are adding.
   */
  void WorkerPool::Add(const std::vector<Worker*>& workers)
  {
    try
    {
      // if the worker has completed all the work they had to
      // then we cannot restart the running thread.
      if (Completed())
      {
        return;
      }

      // add all the workers at once.
      AddToWorkersWaitingToStart(workers);

      // finally make sure that the thread is now up and running.
      if (nullptr == _thread)
      {
        _thread = new Thread(*this);
      }
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
    }
  }

  /**
   * \brief wait for all the workers to complete.
   *        but if they are not stopped then we will timeout
   * \param timeout the number of ms we want to wait for the workers to complete.
   * \return either timeout of complete if the threads completed.
   */
  WaitResult WorkerPool::WaitFor(const long long timeout)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // make sure that we have started/stopped everything
      ProcessThreadsAndWorkersWaiting();

      // make a copy of all the running workers.
      auto runningWorkers = CloneRunningWorkers();
      std::vector<Worker*> remove;

      // send an update for all of them at the same time
      MYODDWEB_MUTEX lock;
      std::for_each(
        std::execution::par,
        runningWorkers.begin(),
        runningWorkers.end(),
        [timeout, &lock, &remove](Worker* worker)
        {
          if( !Wait::SpinUntil([&]()
          {
            if (!worker->Completed())
            {
              return false;
            }
            return true;
          }, timeout))
          {
            MYODDWEB_LOCK(lock);
            remove.emplace_back(worker);
          }
        }
      );

      // if we found anything to be removed we need to process them.
      ProcessWorkersWaitingToEnd(remove);

      // if we removed them all then we completed them all
      // otherwise it means that we didn't complete them all and we timed out.
      return remove.size() == runningWorkers.size() ? WaitResult::complete : WaitResult::timeout;
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
      return WaitResult::timeout;
    }
  }

  /**
   * \brief wait for an array of workers to complete.
   * \param workers the workers we are waiting for.
   * \param timeout how long we are waiting for.
   * \return if any of them timed out.
   */
  WaitResult WorkerPool::WaitFor(const std::vector<Worker*>& workers, long long timeout)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // start whatever needs to start
      ProcessThreadsAndWorkersWaiting();

      // then go around trying to lock th
      MYODDWEB_MUTEX lock;
      auto status = WaitResult::complete;
      // send an update for all of them at the same time
      std::for_each(
        std::execution::par,
        workers.begin(),
        workers.end(),
        [this, timeout,&lock,&status]( Worker* worker )
        {
          const auto result = WaitFor(*worker, timeout);
          if (WaitResult::complete != result)
          {
            MYODDWEB_LOCK(lock);
            status = result;
          }
          std::this_thread::yield();
        }
      );

      // all done
      return status;
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
      return WaitResult::timeout;
    }
  }

  /**
   * \brief wait a little bit for a worker to finish
   *        if the worker does not exist we just return that it is complete.
   * \param worker the worker we are waiting for
   * \param timeout the number of ms we want to wait for the thread to complete.
   * \return either timeout of complete if the thread completed.
   */
  WaitResult WorkerPool::WaitFor(Worker& worker, const long long timeout)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // start those that have yet to start
      ProcessThreadsAndWorkersWaiting();

      // if the worker started between the prvious line and this line
      // it will now be in our list ... or it will not be here at all.
      const auto runningWorkers = CloneRunningWorkers();

      // we need to look for this item
      const auto it = std::find( std::begin(runningWorkers), std::end( runningWorkers ), &worker);
      if (it == runningWorkers.end())
      {
        // it is not in our list, we can assume it was stopped already.
        return WaitResult::complete;
      }

      return
        Wait::SpinUntil([&]()
          {
            // if it is complete, no need to remove it from our list of workers
            // it will be picked up by the next Update( ... ) call
            return worker.Completed();
          }, timeout) ? WaitResult::complete : WaitResult::timeout;
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
      return WaitResult::timeout;
    }
  }
  #pragma endregion

  #pragma region private functions
  /**
   * \brief check if the time has now elapsed.
   * \param givenElapsedTimeMilliseconds the number of ms since the last time we checked.
   * \param actualElapsedTimeMilliseconds the number of ms that has expired since our last check.
   * \return if the time has elapsed and we can continue.
   */
  bool WorkerPool::HasElapsed(float givenElapsedTimeMilliseconds, float& actualElapsedTimeMilliseconds)
  {
    _elapsedTimeMilliseconds += givenElapsedTimeMilliseconds;
    actualElapsedTimeMilliseconds = _elapsedTimeMilliseconds;
    if (_elapsedTimeMilliseconds < static_cast<float>(_throttleElapsedTimeMilliseconds))
    {
      return false;
    }

    //  restart the timer.
    while (_elapsedTimeMilliseconds > static_cast<float>(_throttleElapsedTimeMilliseconds)) {
      _elapsedTimeMilliseconds -= static_cast<float>(_throttleElapsedTimeMilliseconds);
    }
    return true;
  }

  /**
   * \brief Give the worker a chance to do something in the loop
   *        Workers can do _all_ the work at once and simply return false
   *        or if they have a tight look they can return true until they need to come out.
   * \param worker the worker we are managing.
   * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
   * \return true if we want to continue or false if we want to end the thread
   */
  bool WorkerPool::WorkerUpdateOnce(Worker& worker, const float fElapsedTimeMilliseconds)
  {
    //  if we are complete, just remove us and stop processing
    if (worker.Completed())
    {
      // just remove it from the list, as it is complete there is nothing more to do.
      RemoveWorkerFromRunningWorkers(worker);
      return false;
    }

    // then call the worker update
    // if we return true then we want to continue.
    if (worker.WorkerUpdateOnce(fElapsedTimeMilliseconds))
    {
      // this worker is still running
      // so we can add it back to the list of running workers
      AddToRunningWorkers(worker);
      return true;
    }

    // remove it from our list
    // this worker was probably removed by the caller function
    RemoveWorkerFromRunningWorkers(worker);

    // queue this worker now.
    QueueWorkerEnd(worker);

    // we are done with this worker.
    return false;
  }

  /**
   * \brief queue a worker to the end thread
   * \param worker the worker we want to end.
   */
  void WorkerPool::QueueWorkerEnd(Worker& worker)
  {
    // the worker wants to end so we can complete the thread
    // because this is a blocking call, we do not want to wait for it
    // otherwise it will block us all.
    MYODDWEB_LOCK(_lockThreadsWaitingToEnd);

    // and add it to the queue.
    _threadsWaitingToEnd.emplace_back( &worker);
  }

  /**
   * \brief start any workers and thread that need to be started/removed.
   */
  void WorkerPool::ProcessThreadsAndWorkersWaiting()
  {
    MYODDWEB_PROFILE_FUNCTION();

    // do we have _anything_ at all to do?
    // if we have no thread, there is no way we could be ending.
    if( nullptr == _thread)
    {
      return;
    }

    // if we are still in "unknown state" then it means we have not even started yet.
    // so before we go anywhere, we have to wait for the thread to start.
    if( !_thread->Started() )
    {
      if( !Wait::SpinUntil( [&]
      {
        return _thread->Started();
      }, MYODDWEB_WAITFOR_WORKER_COMPLETION ) )
      {
        Logger::Log( 0, LogLevel::Error, L"Workpool unable to start Thread!" );
        return;
      }
    }

    // all the workers waiting to start
    ProcessWorkersWaitingToStart();

    // all the ones waiting to stop
    ProcessThreadsWaitingToEnd();
  }

  /**
   * \brief start any workers that are pending.
   */
  void WorkerPool::ProcessThreadsWaitingToEnd()
  {
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lockThreadsWaitingToEnd);
    if( _threadsWaitingToEnd.empty() )
    {
      return;
    }

    // check the ones that are complete and remove them once done.
    auto clone = std::vector<Worker*>();
    clone.reserve(_threadsWaitingToEnd.size());
    for (auto worker : _threadsWaitingToEnd)
    {
      if( worker->Completed() )
      {
        // we are done with this thread
        delete worker;
        continue;
      }
      clone.emplace_back(worker);
    }

    // copy the updated thread.
    _threadsWaitingToEnd = clone;
  }

  /**
   * \brief start any workers that are pending.
   */
  void WorkerPool::ProcessWorkersWaitingToStart()
  {
    MYODDWEB_PROFILE_FUNCTION();
    // whatever happens this workwer is no longer waiting to start
    // remove it now so it is not going to be started again.
    const auto workersToStart = RemoveWorkersFromWorkersWaitingToStart();

    // if we want to stop or there is nothing to process
    // then we can return right away.
    if(MustStop() || workersToStart.empty() )
    {
      return;
    }

    // work with our clonned list.
    for (auto worker : workersToStart)
    {
      // tell the worker to start,
      if (false == worker->WorkerStart())
      {
        // this worker is not running anymore
        // or the caller did not want it to run
        RemoveWorkerFromRunningWorkers(*worker);
      }
      else
      {
        AddToRunningWorkers(*worker);
      }
    }
  }

  /**
   * \brief process workers that has indicated the need to stop.
   * \param workers the workers we are wanting to stop
   */
  void WorkerPool::ProcessWorkersWaitingToEnd( const std::vector<Worker*>& workers)
  {
    MYODDWEB_PROFILE_FUNCTION();
    if(workers.empty())
    {
      return;
    }

    // this worker might already have been deleted /removed
    // if it was then we cannot try and remove it again.
    try
    {
      // those workers are no longer running
      const auto actualWorkers = RemoveWorkersFromRunningWorkers(workers);

      // call workend for all of them.
      for (auto worker : actualWorkers )
      {
        QueueWorkerEnd(*worker);
      }
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
    }
  }
  #pragma endregion

  #pragma region Thread safe Clone Workers
  /**
   * \brief make a thread safe copy of the running workers.
   */
  std::vector<Worker*> WorkerPool::CloneRunningWorkers()
  {
    MYODDWEB_LOCK(_lockRunningWorkers);
    return std::vector<Worker*>(_runningWorkers);
  }
  #pragma endregion

  #pragma region Thread safe Remove Workers
  /**
   * \brief remove a workers from our list of posible waiting workers.
   *        we will obtain the lock to remove those items.
   * \return the list of items removed.
   */
  std::vector<Worker*> WorkerPool::RemoveWorkersFromWorkersWaitingToStart()
  {
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);

    // make a copy of the list
    const auto clone = std::vector<Worker*>{ _workersWaitingToStart };

    // clear that list
    _workersWaitingToStart.clear();

    // return the copy of the list.
    return clone;
  }

  /**
   * \brief remove a worker from our list of running workers.
   *        we will obtain the lock to remove this item.
   * \param worker the worker we are wanting to remove
   * \return if the item was removed or not.
   */
  bool WorkerPool::RemoveWorkerFromRunningWorkers(const Worker& worker)
  {
    MYODDWEB_LOCK(_lockRunningWorkers);
    return RemoveWorker(_runningWorkers, worker);
  }

  /**
   * \brief remove workers from our list of running workers.
   *        we will obtain the lock to remove this items.
   * \param workers the workers we are wanting to remove
   */
  std::vector<Worker*> WorkerPool::RemoveWorkersFromRunningWorkers(const std::vector<Worker*>& workers)
  {
    MYODDWEB_LOCK(_lockRunningWorkers);
    auto clone = std::vector<Worker*>();
    clone.reserve(workers.size());
    for (const auto worker : workers)
    {
      if( !RemoveWorker(_runningWorkers, *worker) )
      {
        continue;
      }
      clone.emplace_back(worker);
    }
    return clone;
  }

  /**
   * \brief remove all the threads that are waiting to end.
   *        we will obtain the lock to remove this items.
   * \return the list of items removed.
   */
  std::vector<Worker*> WorkerPool::RemoveWorkersFromWorkersWaitingToEnd()
  {
    MYODDWEB_LOCK(_lockThreadsWaitingToEnd);

    // copy the list
    const auto clone = _threadsWaitingToEnd;

    // clear that list
    // we do not want to use `shrink_to_fit` as the reserved value
    // will probably be reused.
    _threadsWaitingToEnd.clear();

    // return the copy of the list.
    return clone;
  }

  /**
   * \brief remove workers from our list of running workers.
   *        we will obtain the lock to remove this items.
   * \return the list of items removed.
   */
  std::vector<Worker*> WorkerPool::RemoveWorkersFromRunningWorkers()
  {
    MYODDWEB_LOCK(_lockRunningWorkers);
    // make a copy of the list
    const auto clone = _runningWorkers;

    // clear that list
    // we do not want to use `shrink_to_fit` as the reserved value
    // will probably be reused.
    _runningWorkers.clear();

    // return the copy of the list.
    return clone;
  }

  /**
   * \brief remove a single worker from a collection of workers
   * \param container the collection of workers.
   * \param item the worker we want t remove
   * \return if the worker was found and removed
   */
  bool WorkerPool::RemoveWorker(std::vector<Worker*>& container, const Worker& item)
  {
    auto removed = false;
    for (;;)
    {
      try
      {
        const auto it = std::find(std::begin(container), std::end(container), &item);
        if (it == container.end())
        {
          break;
        }
        removed = true;
        container.erase(it);
      }
      catch (const std::exception& e)
      {
        // log the error
        Logger::Log(LogLevel::Error, L"Caught exception '%hs' in remove worker.", e.what());
      }
    }
    return removed;
  }
  #pragma endregion 

  #pragma region Thread safe Add Workers
  /**
   * \brief add workers to a list of workers that are waiting to start.
   * \param workers the worker we want to add.
   */
  void WorkerPool::AddToWorkersWaitingToStart(const std::vector<Worker*>& workers)
  {
    MYODDWEB_LOCK(_lockRunningWorkers);
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);
    std::vector<Worker*> uniqueWorkers;
    uniqueWorkers.reserve(workers.size());
    for (auto worker : workers)
    {
      // make sure that we do not add duplicates to our starting list.
      // and also in our running list
      // if we prevent duplicates here, then we will prevent duplicates in our working list
      const auto it1 = std::find(std::begin(_workersWaitingToStart), std::end(_workersWaitingToStart), worker);
      if (it1 != _workersWaitingToStart.end())
      {
        // it is already waiting to start.
        return;
      }

      const auto it2 = std::find(std::begin(_runningWorkers), std::end(_runningWorkers), worker);
      if (it2 != _runningWorkers.end())
      {
        // it is already running.
        return;
      }

      // this item was not found either running or about to start
      uniqueWorkers.emplace_back(worker);
    }

    // we can now add those workers to our list of workers.
    AddWorkers(_workersWaitingToStart, uniqueWorkers);
  }

  /**
   * \brief add this worker to our list of running workers
   * \param worker the worker we are adding
   */
   void WorkerPool::AddToRunningWorkers(Worker& worker)
   {
     MYODDWEB_LOCK(_lockRunningWorkers);
     AddWorker(_runningWorkers, worker);
   }

  /**
   * \brief had a worker to the container
   * \param container the container we are adding to
   * \param item the worker we want to add.
   */
  void WorkerPool::AddWorker(std::vector<Worker*>& container, Worker& item)
  {
    try
    {
      container.emplace_back( &item );
    }
    catch (const std::exception& e)
    {
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' in AddWorker", e.what());
    }
  }

  /**
   * \brief had a worker to the container
   * \param container the container we are adding to
   * \param items the workers we want to add.
   */
  void WorkerPool::AddWorkers(std::vector<Worker*>& container, const std::vector<Worker*>& items)
  {
    try
    {
      container.insert(std::end(container), std::begin(items), std::end(items));
    }
    catch (const std::exception& e)
    {
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' in AddWorkers", e.what());
    }
  }
  #pragma endregion

  #pragma region Worker Abstract/derived functions
  /**
   * \brief stop the running thread and wait
   */
  WaitResult WorkerPool::StopAndWait( const long long timeout)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // make sure that we have started what needed to be started
      ProcessThreadsAndWorkersWaiting();

      // clone the current running workers.
      const auto runningWorkers = CloneRunningWorkers();

      // stop all the workers.
      std::for_each(
        std::execution::par,
        runningWorkers.begin(),
        runningWorkers.end(),
        [this, timeout](Worker* worker)
        {
          StopAndWait(*worker, timeout );
        }
      );

      // set the stop flag here.
      // and we want to wait a little for ourself to complete
      return Worker::StopAndWait( timeout );
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
      return WaitResult::timeout;
    }
  }

  /**
   * \brief non blocking call to instruct the thread to stop.
   */
  void WorkerPool::OnWorkerStop()
  {
    try
    {
      MYODDWEB_PROFILE_FUNCTION();

      // tell everybody to stop
      // make sure that we have started what needed to be started
      ProcessThreadsAndWorkersWaiting();

      // we have to make sure that all the running workers are stopped.
      const auto runningWorkers = CloneRunningWorkers();
      StopWorkers( runningWorkers );
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
    }
  }

  /**
   * \brief stop multiple workers
   * \param workers the workers we are waiting for.
   */
  void WorkerPool::StopWorkers(const std::vector<Worker*>& workers)
  {
    MYODDWEB_PROFILE_FUNCTION();
    // tell everybody to stop
    try
    {
      // make sure that we have started what needed to be started
      ProcessThreadsAndWorkersWaiting();

      // stop all the workers, no need for parallel call
      // the stop function is non blocking.
      for( auto worker : workers )
      {
        StopWorker( *worker );
      }
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
    }
  }

  /**
   * \brief non blocking call to instruct the a single worker to stop
   * \param worker the worker we wish to stop.
   */
  void WorkerPool::StopWorker(Worker& worker)
  {
    MYODDWEB_PROFILE_FUNCTION();
    // tell everybody to stop
    try
    {
      // make sure that we have started what needed to be started
      ProcessThreadsAndWorkersWaiting();

      if (worker.Completed())
      {
        // somehow it is already completed so there is nothing for us to do.
        // we do not remove it here as it will be picked up on the next update
        return;
      }

      // tell the worker to stop then.
      worker.Stop();
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
    }
  }

  /**
   * \brief called when the worker thread is about to start
   */
  bool WorkerPool::OnWorkerStart()
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      //  process anything waiting to start
      ProcessThreadsAndWorkersWaiting();

      // we need to use the lock to make sure that nothing else
      // changes the current running workers.
      // if we have nothing running at all then no point going any further.
      return !CanStopWorkerpoolUpdates();
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
      return false;
    }
  }

  /**
   * \brief Give the worker a chance to do something in the loop
   *        Workers can do _all_ the work at once and simply return false
   *        or if they have a tight look they can return true until they need to come out.
   * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
   * \return true if we want to continue or false if we want to end the thread
   */
  bool WorkerPool::OnWorkerUpdate(const float fElapsedTimeMilliseconds)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // make sure all the ones waiting to start have started
      ProcessThreadsAndWorkersWaiting();

      WorkerEndThreadsWaitingToEnd();

      // the actual elapsed time expired
      float actualElapsedTimeMilliseconds;

      // check if the elapsed time is more than what we want.
      if( !HasElapsed(fElapsedTimeMilliseconds, actualElapsedTimeMilliseconds) )
      {
        return !CanStopWorkerpoolUpdates();
      }

      // take all the running workers and remove them from the list
      // from now on, we will be managing them.
      auto runningWorkers = RemoveWorkersFromRunningWorkers();

      // assume that all the worker want to continue
      // unless of course we have no running worker left
      // then in that case we will need to check
      auto checkIfContinueWithNextEvent = runningWorkers.empty();

      // send an update for all of them at the same time
      std::for_each(
        std::execution::par,
        runningWorkers.begin(),
        runningWorkers.end(),
        [this, actualElapsedTimeMilliseconds, &checkIfContinueWithNextEvent](Worker* worker)
        {
          if (!WorkerUpdateOnce(*worker, actualElapsedTimeMilliseconds))
          {
            // on of them stopped, so we need to check if we need to stop or not.
            // boolean assignments are atomic so we do not need a lock
            // by the time we read the data no threads will be updating it.
            checkIfContinueWithNextEvent = true;
          }
        }
      );

      // if none of the wokrers wanted to stop then we 
      // just want to continue unless we must stop all.
      // there is not point in waiting for ever
      if (!checkIfContinueWithNextEvent)
      {
        // if we are here then we know we have workers
        // still active so we don't need to check any further
        return true;
      }

      // we either had no working workers
      // or we removed at least one of them
      // so we need to check all the others.
      return !CanStopWorkerpoolUpdates();
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
      return !CanStopWorkerpoolUpdates();
    }
  }

  /**
 * \brief if the running worker container is empty or not.
 */
  bool WorkerPool::IsRunningWorkerContainerEmpty()
  {
    // if we have one of more running worker left, then we have to continue
    MYODDWEB_LOCK(_lockRunningWorkers);
    return _runningWorkers.empty();
  }

  /**
 * \brief if the worker waiting to start container is empty or not.
 * \return if we still have workers waiting to start or not
 */
  bool WorkerPool::IsWorkersWaitingToStartContainerEmpty()
  {
    // if we have one or more waiting to start then we have to continiue, (unless told to stop)
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);
    return _workersWaitingToStart.empty();
  }

  /**
   * \brief if the threads waiting to end container is empty or not.
   * \return if we still have threads waiting to end or not
   */
  bool WorkerPool::IsThreadWaitingToEndContainerEmpty()
  {
    MYODDWEB_LOCK(_lockThreadsWaitingToEnd);
    return _threadsWaitingToEnd.empty();
  }

  /**
   * \brief check if we stop or not.
   */
  bool WorkerPool::CanStopWorkerpoolUpdates()
  {
    if( !IsRunningWorkerContainerEmpty() )
    {
      // we still have running workers so we cannot stop.
      return false;
    }

    // if we have one or more waiting to start then we have to continiue, (unless told to stop)
    if (!IsWorkersWaitingToStartContainerEmpty() )
    {
      //  we have workers waiting to start so we cannot stop.
      return false;
    }

    // do we have any threads waiting to end?
    if( !IsThreadWaitingToEndContainerEmpty() )
    {
      //  we have workers waiting to end so we cannot stop.
      return false;
    }

    // we have no running workers, no ending threads and we have no pending worker
    // so we have not more work to do, we can stop
    return true;
  }

  /**
   * \brief Called when the thread pool has been completed, all the workers should have completed here.
   *        We are done with all of them now.
   */
  void WorkerPool::OnWorkerEnd()
  {
    try
    {
      MYODDWEB_PROFILE_FUNCTION();

      // finish all the work of the running workers.
      WorkerEndRunningWorkers();

      // finally we have to wait for all the worker threads to complete
      WorkerEndThreadsWaitingToEnd();
    }
    catch (...)
    {
      // save the exception
      SaveCurrentException();
    }
  }

  /**
   * \brief complete all the running workers
   */
  void WorkerPool::WorkerEndRunningWorkers()
  {
    // remove all the worker and use the list all at once
    // those will need to be completed now so they have to be removed
    auto runningWorkers = RemoveWorkersFromRunningWorkers();
    for (; !runningWorkers.empty();)
    {
      // stop and wait all of them
      std::vector<Worker*> timeOutWorkers;
      MYODDWEB_MUTEX lock;
      std::for_each(
        std::execution::par,
        runningWorkers.begin(),
        runningWorkers.end(),
        [this,&lock,&timeOutWorkers](Worker* worker)
        {
          // stop this worker and wait
          // we cannot end the workpool until the are done.
          const auto result = StopAndWait(*worker, MYODDWEB_WAITFOR_WORKER_COMPLETION);
          if (WaitResult::complete != result)
          {
            MYODDWEB_LOCK(lock);
            timeOutWorkers.emplace_back(worker);
          }
        }
      );

      // copy over whatever we might have left.
      // so we can wait for them to complete.
      runningWorkers = timeOutWorkers;

      // then also add what new workers might have now arrived.
      const auto newRunningWorkers = RemoveWorkersFromRunningWorkers();
      runningWorkers.insert(runningWorkers.end(), newRunningWorkers.begin(), newRunningWorkers.end());
    }
  }

  /**
   * \brief complete all the end threads.
   */
  void WorkerPool::WorkerEndThreadsWaitingToEnd()
  {
    // remove all the worker and use the list all at once
    // those will need to be completed now so they have to be removed
    auto runningWorkers = RemoveWorkersFromWorkersWaitingToEnd();
    for (; !runningWorkers.empty();)
    {
      // stop and wait all of them
      std::for_each(
        std::execution::par,
        runningWorkers.begin(),
        runningWorkers.end(),
        [](Worker* worker)
        {
          if( !worker->Completed() )
          {
            worker->WorkerEnd();
          }

          // this item is complete and can now be deletd.
          // but it is not our ours to delete.
        }
      );

      // then add what new workers might have now arrived.
      runningWorkers = RemoveWorkersFromWorkersWaitingToEnd();
    }
  }
  #pragma endregion 
}
