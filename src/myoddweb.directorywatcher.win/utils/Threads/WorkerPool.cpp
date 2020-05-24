#include "WorkerPool.h"
#include "../../monitors/Base.h"
#include "../Lock.h"
#include "../Wait.h"
#include "CallbackWorker.h"

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

  /**
   * \brief add a worker to our worers pool.
   * \param worker the worker we are trying to add.
   */
  void WorkerPool::Add(Worker& worker)
  {
    MYODDWEB_LOCK(_lock);
    try
    {
      _workers.push_back(&worker);
      if (nullptr == _thread)
      {
        _workersWaitingToStart.push_back(&worker);
        _thread = new Thread(*this);
      }
      else
      {
        // the thread is running already
        // so we can start the items.
        if (!_mustStop)
        {
          worker.WorkerStart();
          _runningWorkers.push_back(&worker);
        }
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

      MYODDWEB_LOCK(_lock);
      for (auto worker : _runningWorkers)
      {
        if (WaitResult::complete != WaitFor(*worker, timeout))
        {
          status = WaitResult::timeout;
        }
      }
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
      // this will prevent it from starting while we release the lock.
      RemoveWorkerFromWorkers(worker);
      RemoveWorkerFromWorkersWaitingStarting(worker);

      // make a copy of our list
      const auto runningWorkers = CloneRunningWorkers();

      // we need to look for this item
      const auto it = std::find(runningWorkers.begin(), runningWorkers.end(), &worker);
      if (it == runningWorkers.end())
      {
        // it was removed already.
        return WaitResult::complete;
      }

      // stop it
      return StopAndRemoveWorker(worker, timeout);
    }
    catch( ... )
    {
      return WaitResult::timeout;
    }
  }

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

  bool WorkerPool::OnWorkerStart()
  {
    try
    {
      //  process anything waiting to start
      ProcessWorkersWaitingToStart();

      // did anything start?
      MYODDWEB_LOCK(_lock);
      // was anything started?
      return !_runningWorkers.empty();
    }
    catch( ... )
    {
      // log it somewhere
    }
  }

  /**
   * \brief start any workers that are pending.
   */
  void WorkerPool::ProcessWorkersWaitingToStart()
  {
    if(_mustStop)
    {
      return;
    }

    // get the ones waiting to start
    const auto workersToStart = CloneWorkersWaitingToStart();
    if( workersToStart.empty() )
    {
      return;
    }

    // then create a list of the ones that started and the ones that did not.
    std::vector<Worker*> remove;
    std::vector<Worker*> add;
    for (auto element : workersToStart)
    {
      if (false == (*element).WorkerStart())
      {
        remove.push_back(element);
      }
      else
      {
        add.push_back(element);
      }
    }

    for (auto worker : remove)
    {
      // we need the lock so we can remove it everywhere
      MYODDWEB_LOCK(_lock);

      // then remove it from all the containers
      // because this item is never going to run here.
      RemoveWorker(_runningWorkers, *worker);
      RemoveWorker(_workersWaitingToStart, *worker);
      RemoveWorker(_workers, *worker);
    }

    for (auto worker : add)
    {
      RemoveWorkerFromWorkersWaitingStarting( *worker);

      // we need the lock
      MYODDWEB_LOCK(_lock);
      _runningWorkers.push_back(worker);
    }
  }

  /**
   * \brief make a thread safe copy of the running workers.
   */
  std::vector<Worker*> WorkerPool::CloneRunningWorkers()
  {
    MYODDWEB_LOCK(_lock);
    return std::vector<Worker*>(_runningWorkers);
  }

  /**
   * \brief make a thread safe copy of the running workers.
   */
  std::vector<Worker*> WorkerPool::CloneWorkersWaitingToStart()
  {
    MYODDWEB_LOCK(_lock);
    return std::vector<Worker*>(_workersWaitingToStart);
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

      // then update them all.
      // if any of the workers return false then we need to process them
      // assume that we must stop
      auto mustStop = true;
      auto runningWorkers = CloneRunningWorkers();
      std::vector<Worker*> remove;
      for (auto element : runningWorkers)
      {
        if (element->Completed() || false == element->OnWorkerUpdate(fElapsedTimeMilliseconds))
        {
          // because this worker returned false
          // then it means we want to get out and remove it.
          remove.push_back(element);
        }
        else
        {
          // for at least one of them we must not stop.
          mustStop = false;
        }
      }

      ProcessWorkersWaitingToStop(remove);

      return !(_mustStop || mustStop);
    }
    catch( ... )
    {
      return !_mustStop;
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
    MYODDWEB_LOCK(_lock);
    auto ts = std::vector<Thread*>();
    try
    {
      for( auto worker : workers )
      {
        try
        {
          auto actual = std::find(_runningWorkers.begin(), _runningWorkers.end(), worker);
          if( actual == _runningWorkers.end() )
          {
            continue;
          }

          std::function<void()> callback = [=]
          {
            if (!worker->Completed())
            {
              worker->WorkerEnd();
            }
          };
          ts.push_back( new Thread( callback ));
        }
        catch (...)
        {
          // @todo we need to log this somewhere.
        }
      }
    
      for( auto t : ts )
      {
        t->WaitFor(MYODDWEB_WAITFOR_WORKER_COMPLETION);
        delete t;
      }

      for (auto worker : workers)
      {
        RemoveWorkerFromRunningWorkers(*worker);
        RemoveWorkerFromWorkers(*worker);
      }
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
    }
  }

  void WorkerPool::OnWorkerEnd()
  {
    // tell everybody to stop
    MYODDWEB_LOCK(_lock);
    auto ts = std::vector<Thread*>();
    try
    {
      for (auto worker : _runningWorkers)
      {
        try
        {
          std::function<void()> callback = [=]
          {
            worker->StopAndWait(MYODDWEB_WAITFOR_WORKER_COMPLETION);
          };
          ts.push_back(new Thread(callback));
        }
        catch (...)
        {
          // @todo we need to log this somewhere.
        }
      }

      for (auto t : ts)
      {
        t->WaitFor(MYODDWEB_WAITFOR_WORKER_COMPLETION);
        delete t;
      }

      // whatever happens we are now done.
      _runningWorkers.clear();
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
    }
  }

  /**
   * \brief non blocking call to instruct the thread to stop.
   */
  void WorkerPool::Stop()
  {
    // tell everybody to stop
    MYODDWEB_LOCK(_lock);
    auto ts = std::vector<Thread*>();
    try
    {
      for (auto worker : _runningWorkers)
      {
        try
        {
          std::function<void()> callback = [=]
          {
            if (!worker->Completed())
            {
              worker->Stop();
            }
          };
          ts.push_back(new Thread(callback));
        }
        catch (...)
        {
          // @todo we need to log this somewhere.
        }
      }

      for (auto t : ts)
      {
        t->WaitFor(MYODDWEB_WAITFOR_WORKER_COMPLETION);
        delete t;
      }
    }
    catch (...)
    {
      // @todo we need to log this somewhere.
    }

    // set the stop flag here.
    _mustStop = true;
  }

  /**
   * \brief remove a worker from our list of posible waiting workers.
   *        we will obtain the lock to remove this item.
   * \param worker the worker we are wanting to remove
   */
  void WorkerPool::RemoveWorkerFromWorkersWaitingStarting(const Worker& worker)
  {
    MYODDWEB_LOCK(_lock);
    RemoveWorker(_workersWaitingToStart, worker);
  }

  /**
   * \brief remove a worker from our list of workers.
   *        we will obtain the lock to remove this item.
   * \param worker the worker we are wanting to remove
   */
  void WorkerPool::RemoveWorkerFromWorkers(const Worker& worker)
  {
    MYODDWEB_LOCK(_lock);
    RemoveWorker(_workers, worker);
  }

  /**
   * \brief remove a worker from our list of running workers.
   *        we will obtain the lock to remove this item.
   * \param worker the worker we are wanting to remove
   */
  void WorkerPool::RemoveWorkerFromRunningWorkers(const Worker& worker)
  {
    MYODDWEB_LOCK(_lock);
    RemoveWorker(_runningWorkers, worker);
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
        // log somewhere
      }
    }
  }
}
