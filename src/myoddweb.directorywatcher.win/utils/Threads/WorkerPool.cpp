// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "WorkerPool.h"
#include "../../monitors/Base.h"
#include "../Lock.h"
#include "../Wait.h"
#include <execution>

#include "../Instrumentor.h"

namespace myoddweb :: directorywatcher :: threads
{
  WorkerPool::WorkerPool() :
    _thread(nullptr)
  {
  }

  WorkerPool::~WorkerPool()
  {
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
      std::recursive_mutex lock;
      auto status = WaitResult::complete;
      // send an update for all of them at the same time
      std::for_each(
        std::execution::par,
        workers.begin(),
        workers.end(),
        [&](auto&& item)
        {
          const auto result = StopAndWait(*item, timeout );
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
    catch( ... )
    {
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
      // log somewhere
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
      std::recursive_mutex lock;
      std::for_each(
        std::execution::par,
        runningWorkers.begin(),
        runningWorkers.end(),
        [&](auto&& item)
        {
          Wait::SpinUntil([&]()
            {
              if (!item->Completed())
              {
                return false;
              }
              MYODDWEB_LOCK(lock);
              remove.push_back(item);
              return true;
            }, timeout);
        }
      );

      // if we found anything to be removed we need to process them.
      ProcessWorkersWaitingToEnd(remove);

      // if we removed them all then we completed them all
      // otherwise it means that we didn't complete them all and we timed out.
      return remove.size() == runningWorkers.size() ? WaitResult::complete : WaitResult::timeout;
    }
    catch( ... )
    {
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
      std::recursive_mutex lock;
      auto status = WaitResult::complete;
      // send an update for all of them at the same time
      std::for_each(
        std::execution::par,
        workers.begin(),
        workers.end(),
        [&](auto&& item)
        {
          const auto result = WaitFor(*item, timeout);
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
    catch( ... )
    {
      return WaitResult::timeout;
    }
  }
  #pragma endregion

  #pragma region private functions
  /**
   * \brief Give the worker a chance to do something in the loop
   *        Workers can do _all_ the work at once and simply return false
   *        or if they have a tight look they can return true until they need to come out.
   * \param worker the worker we are managing.
   * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
   * \return true if we want to continue or false if we want to end the thread
   */
  bool WorkerPool::WorkerUpdate(Worker& worker, const float fElapsedTimeMilliseconds)
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
    if (worker.OnWorkerUpdate(fElapsedTimeMilliseconds))
    {
      // this worker is still running
      // so we can add it back to the list of running workers
      AddToRunningWorkers(worker);
      return true;
    }

    // remove it from our list
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

    // create a new thread
    const auto end = new Thread([&] { worker.WorkerEnd(); });

    // and add it to the queue.
    _threadsWaitingToEnd.push_back(end);
  }

  /**
   * \brief start any workers and thread that need to be started/removed.
   */
  void WorkerPool::ProcessThreadsAndWorkersWaiting()
  {
    MYODDWEB_PROFILE_FUNCTION();

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
    auto clone = std::vector<Thread*>();
    for (auto thread : _threadsWaitingToEnd)
    {
      if( thread->Completed() )
      {
        // we are done with this thread
        delete thread;
        continue;
      }
      clone.push_back(thread);
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
  void WorkerPool::ProcessWorkersWaitingToEnd(std::vector<Worker*>& workers)
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
      RemoveWorkersFromRunningWorkers(workers);

      // call workend for all of them.
      std::for_each(
        workers.begin(),
        workers.end(),
        [&](auto&& item)
        {
          QueueWorkerEnd(*item);
        }
      );
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
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
   */
  void WorkerPool::RemoveWorkerFromRunningWorkers(const Worker& worker)
  {
    MYODDWEB_LOCK(_lockRunningWorkers);
    RemoveWorker(_runningWorkers, worker);
  }

  /**
   * \brief remove workers from our list of running workers.
   *        we will obtain the lock to remove this items.
   * \param workers the workers we are wanting to remove
   */
  std::vector<Worker*> WorkerPool::RemoveWorkersFromRunningWorkers(const std::vector<Worker*>& workers)
  {
    MYODDWEB_LOCK(_lockRunningWorkers);
    for (const auto worker : workers)
    {
      RemoveWorker(_runningWorkers, *worker);
    }
    return std::vector<Worker*>(workers);
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
    const auto clone = std::vector<Worker*>{ _runningWorkers };

    // clear that list
    _runningWorkers.clear();

    // return the copy of the list.
    return clone;
  }

  /**
   * \brief remove a single worker from a collection of workers
   * \param container the collection of workers.
   * \param item the worker we want t remove
   */
  void WorkerPool::RemoveWorker(std::vector<Worker*>& container, const Worker& item)
  {
    for (;;)
    {
      try
      {
        const auto it = std::find(std::begin(container), std::end(container), &item);
        if (it == container.end())
        {
          break;
        }
        container.erase(it);
      }
      catch( ... )
      {
        //  @TODO: Log somewhere
      }
    }
  }
  #pragma endregion 

  #pragma region Thread safe Add Workers
  /**
   * \brief add workers to a list of workers that are waiting to start.
   * \param workers the worker we want to add.
   */
  void WorkerPool::AddToWorkersWaitingToStart(const std::vector<Worker*>& workers)
  {
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);
    for (auto worker : workers)
    {
      AddWorker(_workersWaitingToStart, *worker);
    }
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
      // make sure that we do not add duplicates.
      const auto it = std::find(std::begin(container), std::end(container), &item);
      if( it != container.end() )
      {
        return;
      }
      container.push_back( &item );
    }
    catch( ... )
    {
      //  @TODO: Log somewhere
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
        [&](auto&& item)
        {
          StopAndWait(*item, timeout );
          std::this_thread::yield();
        }
      );

      // set the stop flag here.
      // and we want to wait a little for ourself to complete
      Worker::Stop();

      return Wait::SpinUntil([&]()
        {
          return Completed();
        }, timeout) ? WaitResult::complete : Worker::StopAndWait(timeout);
    }
    catch ( ... )
    {
      return WaitResult::timeout;
    }
  }

  /**
   * \brief non blocking call to instruct the thread to stop.
   */
  void WorkerPool::OnStop()
  {
    MYODDWEB_PROFILE_FUNCTION();
    // tell everybody to stop
    try
    {
      // make sure that we have started what needed to be started
      ProcessThreadsAndWorkersWaiting();

      // clone the current running workers.
      const auto runningWorkers = CloneRunningWorkers();

      Stop( runningWorkers );

      // set the stop flag here.
      Worker::OnStop();
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
    }
  }

  /**
   * \brief stop multiple workers
   * \param workers the workers we are waiting for.
   */
  void WorkerPool::Stop(const std::vector<Worker*>& workers)
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
        Stop( *worker );
      }
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
    }
  }

  /**
   * \brief non blocking call to instruct the a single worker to stop
   * \param worker the worker we wish to stop.
   */
  void WorkerPool::Stop(Worker& worker)
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
      // @todo we need to log this somewhere.
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
      MYODDWEB_LOCK(_lockRunningWorkers);
      return !_runningWorkers.empty();
    }
    catch (...)
    {
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

      // take all the running workers and remove them from the list
      // from now on, we will be managing them.
      auto runningWorkers = RemoveWorkersFromRunningWorkers();

      // assume that all the worker want to continue
      // unless of course we have no running worker left
      // then in that case we will need to check
      auto checkIfContinueWithNextEvent = runningWorkers.empty();
      std::recursive_mutex lock;

      // send an update for all of them at the same time
      std::for_each(
        std::execution::par,
        runningWorkers.begin(),
        runningWorkers.end(),
        [&](auto&& item)
        {
          if (!WorkerUpdate(*item, fElapsedTimeMilliseconds))
          {
            MYODDWEB_LOCK(lock);

            // on of them stopped, so we need to check if we need to stop or not.
            checkIfContinueWithNextEvent = true;
          }
        }
      );

      // if none of the wokrers wanted to stop then we 
      // just want to continue unless we must stop all.
      // there is not point in waiting for ever
      if (!checkIfContinueWithNextEvent)
      {
        return !MustStop();
      }

      // check if we must stop
      return CheckIfMustStop();
    }
    catch (...)
    {
      return !MustStop();
    }
  }

  /**
   * \brief check if we stop or not.
   */
  bool WorkerPool::CheckIfMustStop()
  {
    // if we have one of more running worker left, then we have to continue
    MYODDWEB_LOCK(_lockRunningWorkers);
    if (!_runningWorkers.empty())
    {
      // we still have running workers.
      return !MustStop();
    }

    // if we have one or more waiting to start then we have to continiue, (unless told to stop)
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);
    if (!_workersWaitingToStart.empty())
    {
      //  we have workers waiting to start
      return !MustStop();
    }

    // do we have any threads waiting to end?
    MYODDWEB_LOCK(_lockThreadsWaitingToEnd);
    if( !_threadsWaitingToEnd.empty() )
    {
      //  we have workers waiting to end
      return !MustStop();
    }

    // we have no running workers and we have no pending worker
    // so we have not more work to do, we can stop
    return false;
  }

  /**
   * \brief Called when the thread pool has been completed, all the workers should have completed here.
   *        We are done with all of them now.
   */
  void WorkerPool::OnWorkerEnd()
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // remove all the worker and use the list all at once
      const auto runningWorkers = RemoveWorkersFromRunningWorkers();

      for (;;)
      {
        // assume that all is done
        auto status = WaitResult::complete;

        // stop and wait all of them 
        std::recursive_mutex lock;
        std::for_each(
          std::execution::par,
          runningWorkers.begin(),
          runningWorkers.end(),
          [&](auto&& item)
          {
            // stop this worker and wait
            // we cannot end the workpool until the are done.
            const auto result = item->StopAndWait(MYODDWEB_WAITFOR_WORKER_COMPLETION);
            if (WaitResult::complete != result)
            {
              status = result;
            }
          }
        );
        if( status == WaitResult::complete )
        {
          break;
        }
      }
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
    }
  }
  #pragma endregion 
}
