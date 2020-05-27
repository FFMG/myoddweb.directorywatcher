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
      _thread->WaitFor(MYODDWEB_WAITFOR_WORKER_COMPLETION);
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
      ProcessWorkersWaitingToStart();

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
      ProcessWorkersWaitingToStart();

      auto result = WaitResult::complete;
      if (!worker.Completed())
      {
        result = worker.StopAndWait(timeout);
      }

      // either way, we have to remove it now
      // we started the stop process for this item
      // so we no longer want it running.
      RemoveWorkerFromRunningWorkers(worker);

      // stop it and wait.
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
    try
    {
      // if the worker has completed all the work they had to
      // then we cannot restart the running thread.
      if( Completed() )
      {
        return;
      }

      // add this guy to the list of workers.
      // that are currently waiting to start.
      AddToWorkersWaitingToStart(worker);

      // finally make sure that the thread is now up and running.
      if (nullptr == _thread)
      {
        _thread = new Thread(*this);
      }
    }
    catch( ... )
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
      // start those that have yet to start
      ProcessWorkersWaitingToStart();

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
      ProcessWorkersWaitingToStop(remove);

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
      ProcessWorkersWaitingToStart();

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
      ProcessWorkersWaitingToStart();

      // if the worker started between the prvious line and this line
      // it will now be in our list ... or it will not be here at all.
      const auto runningWorkers = CloneRunningWorkers();

      // we need to look for this item
      const auto it = std::find(runningWorkers.begin(), runningWorkers.end(), &worker);
      if (it == runningWorkers.end())
      {
        // it is not in our list, we can assume it was stopped already.
        return WaitResult::complete;
      }

      return
        Wait::SpinUntil([&]()
          {
            if (!worker.Completed())
            {
              return false;
            }

            // it is complete so we can remove it from our list
            // but it should have been already
            RemoveWorkerFromRunningWorkers(worker);
            return true;
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
  void WorkerPool::ProcessWorkersWaitingToStop(std::vector<Worker*>& workers)
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
        std::execution::par_unseq,
        workers.begin(),
        workers.end(),
        [&](auto&& item)
        {
          if (!item->Completed() )
          {
            item->WorkerEnd();
          }
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
        auto it = std::find(container.begin(), container.end(), &item);
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
   * \brief add a single worker to a list of workers that are waiting to start.
   * \param worker the worker we want to add.
   */
  void WorkerPool::AddToWorkersWaitingToStart(Worker& worker)
  {
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);
    AddWorker(_workersWaitingToStart, worker);
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
      ProcessWorkersWaitingToStart();

      // clone the current running workers.
      const auto runningWorkers = CloneRunningWorkers();

      // stop all the workers.
      std::for_each(
        std::execution::par_unseq,
        runningWorkers.begin(),
        runningWorkers.end(),
        [&](auto&& item)
        {
          StopAndWait(*item, timeout );
        }
      );

      // set the stop flag here.
      // and we want to waut a little for ourself to complet
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
  void WorkerPool::Stop()
  {
    MYODDWEB_PROFILE_FUNCTION();
    // tell everybody to stop
    try
    {
      // make sure that we have started what needed to be started
      ProcessWorkersWaitingToStart();

      // clone the current running workers.
      const auto runningWorkers = CloneRunningWorkers();

      // stop all the workers.
      std::for_each(
        std::execution::par_unseq,
        runningWorkers.begin(),
        runningWorkers.end(),
        [&](auto&& item)
        {
          Stop(*item);
        }
      );

      // set the stop flag here.
      Worker::Stop();
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
      ProcessWorkersWaitingToStart();

      if (worker.Completed())
      {
        // somehow it is already completed
        // so there is nothing for us to do, just remove it.
        RemoveWorkerFromRunningWorkers(worker);
        return;
      }

      // tell the worker to stop then.
      worker.Stop();

      // set the stop flag here.
      Worker::Stop();
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
      ProcessWorkersWaitingToStart();

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
      ProcessWorkersWaitingToStart();

      auto runningWorkers = CloneRunningWorkers();
      std::vector<Worker*> remove;

      // send an update for all of them at the same time
      std::recursive_mutex lock;
      std::for_each(
        std::execution::par,
        runningWorkers.begin(),
        runningWorkers.end(),
        [&]( auto&& item )
        {
          if (item->Completed() || false == item->OnWorkerUpdate(fElapsedTimeMilliseconds))
          {
            MYODDWEB_LOCK(lock);
            remove.push_back(item);
          }
        }
      );
      
      // if we found anything to be removed we need to process them.
      ProcessWorkersWaitingToStop(remove);

      // does it look like we have stopped everything?
      if (remove.size() == runningWorkers.size() )
      {
        // we will really stop if nothing else is running
        // and if we have nothing else waiting to run.
        MYODDWEB_LOCK(_lockWorkersWaitingToStart);
        if( CloneRunningWorkers().empty() && _workersWaitingToStart.empty() )
        {
          return false;
        }
      }

      // check if we must stop at all.
      // but we cannot stop if we still have workers that have not yet stopped.
      if( MustStop() && CloneRunningWorkers().empty() )
      {
        return false;
      }
      return true;
    }
    catch (...)
    {
      return !MustStop();
    }
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

      // stop and wait all of them 
      std::recursive_mutex lock;
      std::for_each(
        std::execution::par_unseq,
        runningWorkers.begin(),
        runningWorkers.end(),
        [&](auto&& item)
        {
          item->StopAndWait(MYODDWEB_WAITFOR_WORKER_COMPLETION);
        }
      );
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
    }
  }
  #pragma endregion 
}
