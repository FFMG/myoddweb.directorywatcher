#include "WorkerPool.h"
#include "../../monitors/Base.h"
#include "../Lock.h"
#include "../Wait.h"
#include "CallbackWorker.h"
#include <execution>

namespace myoddweb :: directorywatcher :: threads
{
  WorkerPool::WorkerPool() :
    _thread(nullptr),
    _mustStop(false)
  {
  }

  WorkerPool::~WorkerPool()
  {
    _mustStop = true;
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
   * \brief wait a little bit for all the workers to finish
   *        if the worker does not exist we just return that it is complete.
   * \param timeout the number of ms we want to wait for the workers to complete.
   * \return either timeout of complete if the threads completed.
   */
  WaitResult WorkerPool::WaitFor(const long long timeout)
  {
    try
    {
      // start those that have yet to start
      ProcessWorkersWaitingToStart();

      // assume we are done
      auto status = WaitResult::complete;

      // make a copy of our running workers those are the ones we want to stop.
      // anything else that is added afterward is not part of what we are waiting for.
      const auto runningWorkers = CloneRunningWorkers();
      for (auto worker : runningWorkers)
      {
        // if one of them fails then we can say that there was a timeout.
        if (WaitResult::complete != WaitFor(*worker, timeout))
        {
          status = WaitResult::timeout;
        }
      }

      // finally remove those workers from our running workers
      RemoveWorkerFromRunningWorkers(runningWorkers);

      return status;
    }
    catch( ... )
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
    try
    {
      // remove the worker that might be in out list
      // eithet waiting to start and/or was not removed properly
      // this will prevent it from starting.
      RemoveWorkerFromWorkersWaitingToStart(worker);

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

      // stop it now and remove it from our list.
      // return the status of the stop.
      return StopAndRemoveWorker(worker, timeout);
    }
    catch( ... )
    {
      return WaitResult::timeout;
    }
  }
  #pragma endregion

  #pragma region private functions
  /**
   * \brief stop a single worker and wait for it to complete
   * \param worker the worker we are stopping
   * \param timeout how long we want to wait for.
   * \return either timeout or complete
   */
  WaitResult WorkerPool::StopAndRemoveWorker(Worker& worker, const long long timeout )
  {
    try
    {
      // either way, remove it so that we do not call on update
      // while we are busy stopping it here.
      RemoveWorkerFromRunningWorkers(worker);

      // stop it and wait.
      return worker.StopAndWait(timeout);
    }
    catch( ... )
    {
      // log somwhere
      return WaitResult::timeout;
    }
  }

  /**
   * \brief start any workers that are pending.
   */
  void WorkerPool::ProcessWorkersWaitingToStart()
  {
    if(_mustStop)
    {
      // remove everything form the list of items waiting to start.
      RemoveWorkerFromWorkersWaitingToStart(_workersWaitingToStart);
      return;
    }

    // get the ones waiting to start
    const auto workersToStart = CloneWorkersWaitingToStart();
    if( workersToStart.empty() )
    {
      return;
    }

    // work with our clonned list.
    for (auto worker : workersToStart)
    {
      // whatever happens this workwer is no longer waiting to start
      // remove it now so it is not going to be started again.
      RemoveWorkerFromWorkersWaitingToStart(workersToStart);

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
    if(workers.empty())
    {
      return;
    }

    // this worker might already have been deleted /removed
    // if it was then we cannot try and remove it again.
    try
    {
      // those workers are no longer running
      RemoveWorkerFromRunningWorkers(workers);

      // auto ts = std::vector<Thread*>();
      // for( auto worker : workers )
      // {
      //   try
      //   {
      //     std::function<void()> callback = [=]
      //     {
      //       if (!worker->Completed())
      //       {
      //         worker->WorkerEnd();
      //       }
      //     };
      //     ts.push_back( new Thread( callback ));
      //   }
      //   catch (...)
      //   {
      //     // @todo we need to log this somewhere.
      //   }
      // }
    
      // for( auto t : ts )
      // {
      //   t->WaitFor(MYODDWEB_WAITFOR_WORKER_COMPLETION);
      //   delete t;
      // }
      //
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

  /**
   * \brief make a thread safe copy of the running workers.
   */
  std::vector<Worker*> WorkerPool::CloneWorkersWaitingToStart()
  {
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);
    return std::vector<Worker*>(_workersWaitingToStart);
  }
  #pragma endregion

  #pragma region Thread safe Remove Workers
  /**
   * \brief remove a workers from our list of posible waiting workers.
   *        we will obtain the lock to remove those items.
   * \param workers the worker we are wanting to remove
   */
  void WorkerPool::RemoveWorkerFromWorkersWaitingToStart(const std::vector<Worker*>& workers)
  {
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);
    for (const auto worker : workers)
    {
      RemoveWorker(_workersWaitingToStart, *worker);
    }
  }

  /**
   * \brief remove a worker from our list of posible waiting workers.
   *        we will obtain the lock to remove this item.
   * \param worker the worker we are wanting to remove
   */
  void WorkerPool::RemoveWorkerFromWorkersWaitingToStart(const Worker& worker)
  {
    MYODDWEB_LOCK(_lockWorkersWaitingToStart);
    RemoveWorker(_workersWaitingToStart, worker);
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
  void WorkerPool::RemoveWorkerFromRunningWorkers(const std::vector<Worker*>& workers)
  {
    MYODDWEB_LOCK(_lockRunningWorkers);
    for (const auto worker : workers)
    {
      RemoveWorker(_runningWorkers, *worker);
    }
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

  #pragma region Worker Abstract functions
  /**
   * \brief non blocking call to instruct the thread to stop.
   */
  void WorkerPool::Stop()
  {
    // tell everybody to stop
    try
    {
      // clone the current running workers.
      const auto runningWorkers = CloneRunningWorkers();

      // remove those workers
      // because they are not working
      RemoveWorkerFromRunningWorkers(runningWorkers);

      // auto ts = std::vector<Thread*>();
      // for (auto worker : runningWorkers)
      // {
      //   try
      //   {
      //     std::function<void()> callback = [=]
      //     {
      //       if (!worker->Completed())
      //       {
      //         worker->Stop();
      //       }
      //     };
      //     ts.push_back(new Thread(callback));
      //   }
      //   catch (...)
      //   {
      //     // @todo we need to log this somewhere.
      //   }
      // }
      //
      // for (auto t : ts)
      // {
      //   t->WaitFor(MYODDWEB_WAITFOR_WORKER_COMPLETION);
      //   delete t;
      // }

      std::for_each(
        std::execution::par_unseq,
        runningWorkers.begin(),
        runningWorkers.end(),
        [&](auto&& item)
        {
          if (!item->Completed())
          {
            item->Stop();
          }
        }
      );

      // set the stop flag here.
      _mustStop = true;
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
    try
    {
      // make sure all the ones waiting to start have started
      ProcessWorkersWaitingToStart();

      auto runningWorkers = CloneRunningWorkers();
      std::vector<Worker*> remove;

      // send an update for all of them at the same time
      std::recursive_mutex lock;
      std::for_each(
        std::execution::par_unseq,
        runningWorkers.begin(),
        runningWorkers.end(),
        [&]( auto&& item )
        {
          if (item->Completed() || false == item->OnWorkerUpdate(fElapsedTimeMilliseconds))
          {
            auto guard = Lock(lock);
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
        _mustStop = CloneRunningWorkers().empty() && CloneWorkersWaitingToStart().empty();
      }

      // check if we must stop at all.
      return !_mustStop;
    }
    catch (...)
    {
      return !_mustStop;
    }
  }

  /**
   * \brief Called when the thread pool has been completed, all the workers should have completed here.
   *        We are done with all of them now.
   */
  void WorkerPool::OnWorkerEnd()
  {
    try
    {
      // clone the current running workers.
      const auto runningWorkers = CloneRunningWorkers();

      // remove those workers
      // because they are not working
      RemoveWorkerFromRunningWorkers(runningWorkers);

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
