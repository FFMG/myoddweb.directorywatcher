// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "WorkerPool.h"
#include <cassert>
#include <execution>

#include "../Lock.h"
#include "../Logger.h"
#include "../LogLevel.h"
#include "../Wait.h"

namespace myoddweb::directorywatcher::threads
{
  /// <summary>
  /// Called when when the pool is starting
  /// </summary>
  /// <param name="throttleElapsedTimeMilliseconds">How often we want updates to happen</param>
  WorkerPool::WorkerPool( const long long throttleElapsedTimeMilliseconds) :
    _throttleElapsedTimeMilliseconds(static_cast<float>(throttleElapsedTimeMilliseconds)),
    _fElapsedTimeMilliseconds( 0 ),
    _thread( nullptr)
  {
  }

  /// <summary>
  /// The destructor.
  /// </summary>
  /// <returns></returns>
  WorkerPool::~WorkerPool()
  {
    try
    {
      // we need to make sure all is good
      StopAndWait(-1);

      // clen what needs to be
      RemoveAllCompletedWorkers();

      // kill the thread
      DeleteWorkerThreadIfComplete();

#ifdef _DEBUG
      // make sure that the memory is clear.
      assert(_thread == nullptr);
      assert(_addFutures.empty());
      assert(_workerAndFutures.empty());
#endif
    }
    catch (std::exception& e)
    {
      //  there was an error while shutting down
      Logger::Log(Id(), LogLevel::Error, L"Caught exception '%hs' in PublishEvents, check the callback!", e.what());
    }
  }

  #pragma region Helpers
  /// <summary>
  /// Add a worker to the pool of workers.
  /// </summary>
  /// <param name="worker">The worker we want to add.</param>
  void WorkerPool::Add(Worker& worker)
  {
    // just add the worker to our list of workers.
    // we do that in a thread in case we have workers that add within workers.
    MYODDWEB_LOCK(_addFuturesLock);
    _addFutures.push_back( new std::future<void>(std::async(std::launch::async, [this, &worker]
      {
        AddWorker(worker);
      })));
  }

  /// <summary>
  /// Wait for a worker to either complete or timeout
  /// </summary>
  /// <param name="worker">The worker we will be waiting for</param>
  /// <param name="timeout">How long to wait for.</param>
  /// <returns>Either complete or timeout</returns>
  WaitResult WorkerPool::WaitFor(Worker& worker, const long long timeout)
  {
    // wait for everybody to be added
    WaitForAllAddFuturesPending();

    // look for that worker
    if (!Exists(worker))
    {
      // it is not in our list, we can assume it was stopped already.
      return WaitResult::complete;
    }
    StartWorkerThreadIfNeeded();
    const auto wait = worker.WaitFor(  timeout );

    // remove whatever was done
    RemoveAllCompletedWorkers();

    // and we are done
    return wait;
  }

  /// <summary>
  /// Wait for all the workers to either finish or the timeout.
  /// </summary>
  /// <param name="timeout">How long to wait for.</param>
  /// <returns>Either complete or timeout</returns>
  WaitResult WorkerPool::WaitFor(const long long timeout)
  {
    // wait for everybody to be added
    WaitForAllAddFuturesPending();

    StartWorkerThreadIfNeeded();
    const auto wait = Worker::WaitFor( timeout );

    // remove whatever is complete
    RemoveAllCompletedWorkers();

    // and we are done
    return wait;
  }

  /// <summary>
  /// Signal a single worker to stop.
  /// </summary>
  /// <param name="worker"></param>
  void WorkerPool::StopWorker(Worker& worker)
  {
    // wait for everybody to be added
    WaitForAllAddFuturesPending();

    // look for that worker
    if( !Exists(worker ))
    {
      return;
    }
    StartWorkerThreadIfNeeded();
    worker.Stop();
  }

  /// <summary>
  /// Stop one or more workers and wait for them to complete.
  /// After stopping them we wait for them all or timeout.
  /// </summary>
  /// <param name="workers">The workers we are waiting for.</param>
  /// <param name="timeout">How long we want to wait for.</param>
  /// <returns>Either complete if everything completed or timeout</returns>
  WaitResult WorkerPool::StopAndWait(const std::vector<Worker*>& workers, const long long timeout)
  {
    // wait for everybody to be added
    WaitForAllAddFuturesPending();

    // first we will stop the workers
    // we now need to wait for all the processes to finish
    std::for_each(
      std::execution::par,
      workers.begin(),
      workers.end(),
      [timeout, this](Worker* worker)
      {
        if (Exists(*worker))
        {
          worker->StopAndWait(timeout);
        }
      }
    );

    // then make sure that the futures are done
    return WaitForAllFuturesToComplete(workers, timeout);
  }

  /// <summary>
  /// Wait for a single engine worker to complete and wait for it to complete.
  /// </summary>
  /// <param name="worker">The worker we are waiting for.</param>
  /// <param name="timeout">How long we want to wait for.</param>
  /// <returns>Either complete if everything completed or timeout</returns>
  WaitResult WorkerPool::StopAndWait(Worker& worker, const long long timeout)
  {
    return StopAndWait({&worker}, timeout );
  }

  /// <summary>
  /// Stop alll the workers and wait for them all to complete.
  /// </summary>
  /// <param name="timeout">The number of ms we want to wait for</param>
  /// <returns>Either timeout or complete if all the workers completed</returns>
  WaitResult WorkerPool::StopAndWait( const long long timeout)
  {
    // wait for all the start workers
    WaitForAllAddFuturesPending();

    // just tell all our workers to stop.
    StopAllWorkers();

    // we have to wait for all the futures to complete
    // if they timeout then we cannot stop ourselves.
    if( WaitResult::timeout == WaitForAllFuturesToComplete(timeout) )
    {
      return WaitResult::timeout;
    }

    // now that our futures are complete, (the ones we are aware of)
    // we can call ourselves to stop
    // if we could not complete the futures, then we cannot stop
    return Worker::StopAndWait(timeout);
  }
  #pragma endregion

  #pragma region Worker
  /// <summary>
  /// Called when a stop request was made
  /// We must stop all the worker and prevent any more from being added.
  /// </summary>
  void WorkerPool::OnWorkerStop()
  {
    // send a stop notification to all the workers.
    StopAllWorkers();
  }

  /// <summary>
  /// Called when when the pool is starting
  /// </summary>
  /// <returns></returns>
  bool WorkerPool::OnWorkerStart()
  {
    // wait for all the start workers
    WaitForAllAddFuturesPending();

    for ( const auto workerAndFuture : _workerAndFutures)
    {
      const auto worker = workerAndFuture.first;
      if( worker->Started() || worker->Completed())
      {
        continue;
      }

      if( !worker->WorkerStart() )
      {
        // it does not want to start so it has to be completed.
        assert(worker->Completed());
      }
    }
    return true;
  }

  /// <summary>
  /// Called at regular intervals
  /// </summary>
  /// <param name="fElapsedTimeMilliseconds"></param>
  /// <returns>False if we want to end the pool or true if we want to continue</returns>
  bool WorkerPool::OnWorkerUpdate(const float fElapsedTimeMilliseconds)
  {
    // wait for all the start workers
    WaitForAllAddFuturesPending();

    // assume that none of our workers want to continue
    // ignore the completed workers.
    auto mustContinue = false;

    // update our elasped time
    _fElapsedTimeMilliseconds += fElapsedTimeMilliseconds;

    MYODDWEB_LOCK(_workerAndFuturesLock);
    for (const auto workerAndFuture : _workerAndFutures)
    {
      // the worker
      const auto worker = workerAndFuture.first;

      // if that worker is completed then we do not care
      // it will be removed at some other poing
      if (worker->Completed())
      {
        continue;
      }

      // check if this worker has started
      if (!worker->Started())
      {
        // does it wants to start
        if (!worker->WorkerStart())
        {
          // it does not want to start so it has to be completed.
          // we do not change the mustContinue flag in case
          // another worker wants to continue.
          assert(worker->Completed());
          continue;
        }
      }

      // while we are in the quick update loop, we want to check if the future returned false,
      // or if we completed our end workers.
      // if it did, then we need to end it right away rather than waiting for the next loop.
      const auto end = GetEndFutureEndStateInLock(*worker);
      if (FutureEndState::CompleteTrue == end )
      {
        // we are now completely done with this worker
        // all the updates and end futures have been called.
        // we don't change the flag in case someone else wants to continue
        continue;
      }

      if (FutureEndState::StillRunning == end )
      {
        // this is still running, so we want to go on.
        mustContinue = true;
        continue;
      }

      const auto update = GetUpdateFutureEndStateInLock(*worker);
      if(FutureEndState::CompleteFalse == update )
      {
        // the worker returned false, so it wants to end
        WorkerEndInLock( *worker );

        // we want to continue, only once the end future is done can we end
        mustContinue = true;
        continue;
      }
      else if (FutureEndState::StillRunning == update)
      {
        // the worker is still busy, so we do not want to call it again
        // so we must go around one more time.
        mustContinue = true;
        continue;
      }

      // if the timeout has expired
      if(_fElapsedTimeMilliseconds < _throttleElapsedTimeMilliseconds )
      {
        mustContinue = true;
        continue;
      }

      // we can now call the update
      if (!UpdateOnceInLock( *worker, _fElapsedTimeMilliseconds ))
      {
        WorkerEndInLock(*worker);

        // we want to continue, only once the end future is done can we end
        mustContinue = true;
        continue;
      }

      // because at least one worker wants to continue
      // so we will go forward once more.
      mustContinue = true;
    }

    // did we go over our elapsed time?
    if (_fElapsedTimeMilliseconds >= _throttleElapsedTimeMilliseconds)
    {
      _fElapsedTimeMilliseconds = 0;
    }

    // return if we must continue or not or if we still have pending futures.
    return mustContinue || HasAddFuturesPending();
  }

  /// <summary>
  /// When the worker pool has ended.
  /// </summary>
  void WorkerPool::OnWorkerEnd()
  {
    // wait for all the start workers
    WaitForAllAddFuturesPending();

    MYODDWEB_LOCK(_workerAndFuturesLock);
    for (const auto workerAndFutures : _workerAndFutures)
    {
      const auto worker = workerAndFutures.first;

      // if our worker is still running then we cannot end the worker
      if( FutureEndState::StillRunning == GetUpdateFutureEndStateInLock(*worker))
      {
        continue;
      }

      // no need to check if stopped already or not
      // the worker class checks if we can call it.
      WorkerEndInLock( *worker );
    }
  }
  #pragma endregion

  #pragma region Private Helpers
  /// <summary>
  /// Get the future for a worker while we have the lock
  /// We return null if we do not have one.
  /// </summary>
  /// <param name="worker">The worker we are looking for</param>
  /// <returns>The future</returns>
  WorkerPool::Futures* WorkerPool::GetFuturesWorkerInLock(Worker& worker) const
  {
    // look for this worker in our map
    const auto it = _workerAndFutures.find(&worker);
    if (it == _workerAndFutures.end())
    {
      return nullptr;
    }

    // return the current future
    return it->second;
  }

  /// <summary>
  /// Send a motification to stop all the workers.
  /// </summary>
  void WorkerPool::StopAllWorkers()
  {
    MYODDWEB_LOCK(_workerAndFuturesLock);
    for (const auto workerAndFutures : _workerAndFutures)
    {
      auto worker = workerAndFutures.first;
      // no need to check if stopped already or not
      // the worker class checks if we can call it.
      worker->Stop();
    }
  }

  /// <summary>
  /// Delete the worker thread if the work is complete
  /// So that it can be re-used if needed.
  /// </summary>
  void WorkerPool::DeleteWorkerThreadIfComplete()
  {
    if (!Is(State::complete))
    {
      return;
    }
    MYODDWEB_LOCK(_threadLock);
    delete _thread;
    _thread = nullptr;
    _fElapsedTimeMilliseconds = 0;
    SetState(State::unknown);
  }

  /// <summary>
  /// Safely start the worker thread if needed.
  /// </summary>
  void WorkerPool::StartWorkerThreadIfNeeded()
  {
    MYODDWEB_LOCK(_threadLock);
    if (_thread != nullptr)
    {
      return;
    }

    // start ourselves as a worker
    _thread = new Thread(*this);
  }

  /// <summary>
  /// Get the current number of running workers
  /// </summary>
  /// <returns>Num number of workers</returns>
  int WorkerPool::NumberOfIncompleteWorkers() const
  {
    MYODDWEB_LOCK(_workerAndFuturesLock);
    auto number = 0;
    for (const auto workerAndFutures : _workerAndFutures)
    {
      const auto worker = workerAndFutures.first;
      if (!worker->Completed())
      {
        continue;
      }
      ++number;
    }
    return number;
  }

  /// <summary>
  /// Safely add a container to the list.
  /// </summary>
  /// <param name="worker">The container to add.</param>
  void WorkerPool::AddWorker(Worker& worker)
  {
    // if the work is complete then we need to restart it
    DeleteWorkerThreadIfComplete();

    MYODDWEB_LOCK(_workerAndFuturesLock);
    // make sure that this worker does not exist already.
    const auto it = _workerAndFutures.find(&worker);
    if( it != _workerAndFutures.end() )
    {
      return;
    }

    _workerAndFutures[&worker] = nullptr;

    // make sure that the thread is running
    StartWorkerThreadIfNeeded();
  }

  /// <summary>
  /// Check if the worker is one of our workers
  /// </summary>
  /// <param name="worker">The worker we are looking for.</param>
  /// <returns></returns>
  bool WorkerPool::Exists(Worker& worker) const
  {
    MYODDWEB_LOCK(_workerAndFuturesLock);
    const auto it = _workerAndFutures.find(&worker);
    return it != _workerAndFutures.end();
  }

  /// <summary>
  /// Get the state of the future, (complete, running, ...)
  /// We also delete the future if it is no longer needed.
  /// </summary>
  /// <param name="worker"></param>
  /// <returns></returns>
  WorkerPool::FutureEndState WorkerPool::GetUpdateFutureEndStateInLock(Worker& worker) const
  {
    // look for the current future
    auto currentFutures = GetFuturesWorkerInLock(worker);
    if (currentFutures == nullptr || currentFutures->_update == nullptr)
    {
      // the future us not running at all.
      return FutureEndState::NotRunning;
    }

    // is it valid?
    if (!currentFutures->_update->valid())
    {
      // no, it is no longer valid, we need to add one.
      delete currentFutures->_update;
      currentFutures->_update = nullptr;
      return FutureEndState::NotRunning;
    }

    // so the future for this worker is still running
    // so we want to get a result for it.
    // wait one more ms to see if it needs to be complete.
    const auto wait = std::chrono::milliseconds(1);
    if (currentFutures->_update->wait_for(wait) == std::future_status::ready)
    {
      // it is complete! So we can get the result from it.
      const auto result = currentFutures->_update->get();

      // and remove the old one
      delete currentFutures->_update;
      currentFutures->_update = nullptr;

      // if the result is false, then we do not want to create another future
      return result ? FutureEndState::CompleteTrue : FutureEndState::CompleteFalse;
    }

    // this future is not complete, there is nothing more for us to do.
    // so we return true so we will come back here one more time.
    return FutureEndState::StillRunning;
  }

  /// <summary>
  /// Get the state of the future, (complete, running, ...)
  /// We also delete the future if it is no longer needed.
  /// </summary>
  /// <param name="worker"></param>
  /// <returns></returns>
  WorkerPool::FutureEndState WorkerPool::GetEndFutureEndStateInLock(Worker& worker) const
  {
    // look for the current future
    auto currentFutures = GetFuturesWorkerInLock(worker);
    if (currentFutures == nullptr || currentFutures->_end == nullptr)
    {
      // the future us not running at all.
      return FutureEndState::NotRunning;
    }

    // is it valid?
    if (!currentFutures->_end->valid())
    {
      // no, it is no longer valid, we need to add one.
      delete currentFutures->_end;
      currentFutures->_end = nullptr;
      return FutureEndState::NotRunning;
    }

    // so the future for this worker is still running
    // so we want to get a result for it.
    // wait one more ms to see if it needs to be complete.
    const auto wait = std::chrono::milliseconds(1);
    if (currentFutures->_end->wait_for(wait) == std::future_status::ready)
    {
      // it is complete! So we can get the result from it.
      currentFutures->_end->get();

      // and remove the old one
      delete currentFutures->_end;
      currentFutures->_end = nullptr;

      // if the result is false, then we do not want to create another future
      return FutureEndState::CompleteTrue;
    }

    // this future is not complete, there is nothing more for us to do.
    // so we return true so we will come back here one more time.
    return FutureEndState::StillRunning;
  }

  /// <summary>
  /// Call the worker end for this worker and create a future for it.
  /// </summary>
  /// <param name="worker"></param>
  void WorkerPool::WorkerEndInLock(Worker& worker)
  {
    // if we are not started we do not want to end
    if (!worker.Started())
    {
      return;
    }

    // get the futures
    auto futures = _workerAndFutures[&worker];
    if(futures != nullptr && futures->_end != nullptr )
    {
      // we called the end already
      return;
    }

    // get the future we will be calling
    const auto newFuture = new std::future<void>(std::async(std::launch::async, [&worker]
      {
        worker.WorkerEnd();
      }));

    if( nullptr == futures )
    {
      futures = new Futures(nullptr, newFuture);
    }
    else
    {
      // it should have been cleanned up
      assert(futures->_update == nullptr);

      // and we should not have an end already running.
      assert(futures->_end == nullptr);

      // set the end future
      futures->SetEnd(newFuture);
    }

    // then save the new value.
    _workerAndFutures[&worker] = futures;
  }

  /// <summary>
  /// Update a single worker in a lock, either check the result or create a result
  /// </summary>
  /// <param name="worker"></param>
  /// <param name="fElapsedTimeMilliseconds"></param>
  /// <returns>True if we want to continue or false if we want to stop.</returns>
  bool WorkerPool::UpdateOnceInLock(Worker& worker, const float fElapsedTimeMilliseconds)
  {
    // get the current future status
    switch( GetUpdateFutureEndStateInLock( worker) )
    {
    case FutureEndState::NotRunning:
    case FutureEndState::CompleteTrue:
      // it is not running or it is complete
      // either way, we want to do it one more time.
      break;

    case FutureEndState::StillRunning:
      // it is still running, so we must return true
      // so we get one more update
      return true;

    case FutureEndState::CompleteFalse:
      // the result was false, so we do not want to continue.
      return false;
    }

    // if we are here then we need to create another future
    const auto newFuture = new std::future<bool>(std::async(std::launch::async, [fElapsedTimeMilliseconds, &worker]
      {
        return worker.WorkerUpdateOnce(fElapsedTimeMilliseconds);
      }));

    // then update the current values.
    auto futures = _workerAndFutures[&worker];
    if (nullptr == futures)
    {
      futures = new Futures(newFuture, nullptr );
    }
    else
    {
      // it should have been cleanned up
      assert(futures->_update == nullptr);

      // set the update future
      futures->SetUpdate( newFuture );
    }

    // then save the new value.
    _workerAndFutures[&worker] = futures;

    // and continue
    return true;
  }

  /// <summary>
  /// Wait for all the futures in a list of workers to complete.
  /// </summary>
  /// <param name="workers"></param>
  /// <param name="timeout"></param>
  /// <returns></returns>
  WaitResult WorkerPool::WaitForAllFuturesToComplete(const std::vector<Worker*>& workers, const long long timeout)
  {
    // wait for the operation to complete.
    const auto wait = Wait::SpinUntil([this, &workers]
      {
        auto stillRunning = false;
        MYODDWEB_LOCK(_workerAndFuturesLock);
        for (const auto worker : workers)
        {
          const auto workerAndFuture = _workerAndFutures.find(worker);
          if (workerAndFuture == _workerAndFutures.end())
          {
            continue;
          }

          if (FutureEndState::StillRunning == GetUpdateFutureEndStateInLock(*workerAndFuture->first))
          {
            stillRunning = true;
          }
          if (FutureEndState::StillRunning == GetEndFutureEndStateInLock(*workerAndFuture->first))
          {
            stillRunning = true;
          }
        }
        return !stillRunning;
      }, timeout);

    // while we are here, remove all the completed workers.
    RemoveAllCompletedWorkers();

    // then return if we completed successfully or not.
    return wait ? WaitResult::complete : WaitResult::timeout;
  }

  /// <summary>
  /// Wait for all the workers that still have a future to complete
  /// We do not care about the result, we simply want them to end.
  /// </summary>
  /// <param name="timeout">How long we are prepared to wait for.</param>
  /// <returns></returns>
  WaitResult WorkerPool::WaitForAllFuturesToComplete( const long long timeout)
  {
    // wait for the operation to complete.
    const auto wait = Wait::SpinUntil([this]
      {
        auto stillRunning = false;
        MYODDWEB_LOCK(_workerAndFuturesLock);
        for (const auto workerAndFuture : _workerAndFutures)
        {
          if (FutureEndState::StillRunning == GetUpdateFutureEndStateInLock(*workerAndFuture.first))
          {
            stillRunning = true;
          }
          if (FutureEndState::StillRunning == GetEndFutureEndStateInLock(*workerAndFuture.first))
          {
            stillRunning = true;
          }
        }
        return !stillRunning;
      }, timeout);

    // while we are here, remove all the completed workers.
    RemoveAllCompletedWorkers();

    // then return if we completed successfully or not.
    return wait ? WaitResult::complete : WaitResult::timeout;
  }

  /// <summary>
  /// Remove all the completed workers from the list and free the memories
  /// </summary>
  void WorkerPool::RemoveAllCompletedWorkers()
  {
    MYODDWEB_LOCK(_workerAndFuturesLock);
    std::vector<Worker*> workersToRemove;

    // first look for everything we want to remove
    for (auto workerAndFuture : _workerAndFutures)
    {
      if( !workerAndFuture.first->Completed() )
      {
        continue;
      }
      delete workerAndFuture.second;
      workersToRemove.push_back(workerAndFuture.first);
    }

    // then remove them
    for (auto worker : workersToRemove)
    {
      const auto it = _workerAndFutures.find(worker);
      _workerAndFutures.erase(it);
    }
  }

  /// <summary>
  /// Check if we have any wokers still being added.
  /// </summary>
  /// <returns>If we still have workers.</returns>
  bool WorkerPool::HasAddFuturesPending()
  {
    MYODDWEB_LOCK(_addFuturesLock);
    auto busy = false;
    const auto wait = std::chrono::milliseconds(1);
    auto it = _addFutures.begin();
    while( it != _addFutures.end() )
    {
      auto currentFutures = (*it);
      if( !(*it)->valid())
      {
        delete currentFutures;
        _addFutures.erase(it);
        it = _addFutures.begin();
        continue;
      }
      
      if (currentFutures->wait_for(wait) == std::future_status::ready)
      {
        currentFutures->get();
        delete currentFutures;
        _addFutures.erase(it);
        it = _addFutures.begin();
        continue;
      }

      // if we are her, then we are still busy
      busy = true;

      // check the others if they can be removed.
      ++it;
    }
    return busy;
  }

  /// <summary>
  /// Wait for all the add futures to complete.
  /// </summary>
  void WorkerPool::WaitForAllAddFuturesPending()
  {
    Wait::SpinUntil([this]
    {
      return !HasAddFuturesPending();
    }, -1);

#ifdef _DEBUG
    // make sure that the memory is clear.
    assert(_addFutures.empty());
#endif
  }
  #pragma endregion 
}
