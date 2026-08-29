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
      stop_and_wait(-1);

      // clen what needs to be
      remove_all_completed_workers();

      // kill the thread
      delete_worker_thread_if_complete();

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
      Logger::log(id(), LogLevel::Error, L"Caught exception '%hs' in PublishEvents, check the callback!", e.what());
    }
  }

  /// <summary>
  /// Call the protected/friend-only Worker::worker_end() on the given worker.
  /// </summary>
  void WorkerPool::worker_end_now(Worker& worker) const
  {
    worker.worker_end();
  }

  /// <summary>
  /// Call the protected/friend-only Worker::worker_update_once() on the given worker.
  /// </summary>
  bool WorkerPool::worker_update_once_now(Worker& worker, const float fElapsedTimeMilliseconds) const
  {
    return worker.worker_update_once(fElapsedTimeMilliseconds);
  }

  bool WorkerPool::all_futures_in_list_complete_predicate::operator()() const
  {
    auto stillRunning = false;
    MYODDWEB_LOCK(_pool._workerAndFuturesLock);
    for (const auto& worker : _workers)
    {
      const auto workerAndFuture = _pool._workerAndFutures.find(worker);
      if (workerAndFuture == _pool._workerAndFutures.end())
      {
        continue;
      }

      if (FutureEndState::StillRunning == _pool.get_update_future_end_state_in_lock(*workerAndFuture->first))
      {
        stillRunning = true;
      }
      if (FutureEndState::StillRunning == _pool.get_end_future_end_state_in_lock(*workerAndFuture->first))
      {
        stillRunning = true;
      }
    }
    return !stillRunning;
  }

  bool WorkerPool::all_futures_complete_predicate::operator()() const
  {
    auto stillRunning = false;
    MYODDWEB_LOCK(_pool._workerAndFuturesLock);
    for (const auto& workerAndFuture : _pool._workerAndFutures)
    {
      if (FutureEndState::StillRunning == _pool.get_update_future_end_state_in_lock(*workerAndFuture.first))
      {
        stillRunning = true;
      }
      if (FutureEndState::StillRunning == _pool.get_end_future_end_state_in_lock(*workerAndFuture.first))
      {
        stillRunning = true;
      }
    }
    return !stillRunning;
  }

  #pragma region Helpers
  /// <summary>
  /// Add a worker to the pool of workers.
  /// </summary>
  /// <param name="worker">The worker we want to add.</param>
  void WorkerPool::add(Worker& worker)
  {
    // just add the worker to our list of workers.
    // we do that in a thread in case we have workers that add within workers.
    MYODDWEB_LOCK(_addFuturesLock);
    _addFutures.push_back( new std::future<void>(std::async(std::launch::async, add_worker_task(*this, worker))));
  }

  /// <summary>
  /// Wait for a worker to either complete or timeout
  /// </summary>
  /// <param name="worker">The worker we will be waiting for</param>
  /// <param name="timeout">How long to wait for.</param>
  /// <returns>Either complete or timeout</returns>
  WaitResult WorkerPool::wait_for(Worker& worker, const long long timeout)
  {
    // wait for everybody to be added
    wait_for_all_add_futures_pending();

    // look for that worker
    if (!exists(worker))
    {
      // it is not in our list, we can assume it was stopped already.
      return WaitResult::complete;
    }
    start_worker_thread_if_needed();
    const auto wait = worker.wait_for(  timeout );

    // remove whatever was done
    remove_all_completed_workers();

    // and we are done
    return wait;
  }

  /// <summary>
  /// Wait for all the workers to either finish or the timeout.
  /// </summary>
  /// <param name="timeout">How long to wait for.</param>
  /// <returns>Either complete or timeout</returns>
  WaitResult WorkerPool::wait_for(const long long timeout)
  {
    // wait for everybody to be added
    wait_for_all_add_futures_pending();

    start_worker_thread_if_needed();
    const auto wait = Worker::wait_for( timeout );

    // remove whatever is complete
    remove_all_completed_workers();

    // and we are done
    return wait;
  }

  /// <summary>
  /// Signal a single worker to stop.
  /// </summary>
  /// <param name="worker"></param>
  void WorkerPool::stop_worker(Worker& worker)
  {
    // before we stop all workers ... we have
    // to make sure that everybody is started
    start_all_pending_workers();

    // look for that worker
    if( !exists(worker ))
    {
      return;
    }
    start_worker_thread_if_needed();
    worker.stop();
  }

  /// <summary>
  /// Stop one or more workers and wait for them to complete.
  /// After stopping them we wait for them all or timeout.
  /// </summary>
  /// <param name="workers">The workers we are waiting for.</param>
  /// <param name="timeout">How long we want to wait for.</param>
  /// <returns>Either complete if everything completed or timeout</returns>
  WaitResult WorkerPool::stop_and_wait(const std::vector<Worker*>& workers, const long long timeout)
  {
    // before we stop those workers ... we have
    // to make sure that everybody is started
    start_all_pending_workers();

    // first we will stop the workers
    // we now need to wait for all the processes to finish
    std::for_each(
      std::execution::par,
      workers.begin(),
      workers.end(),
      stop_worker_task(*this, timeout)
    );

    // then make sure that the futures are done
    return wait_for_all_futures_to_complete(workers, timeout);
  }

  /// <summary>
  /// Wait for a single engine worker to complete and wait for it to complete.
  /// </summary>
  /// <param name="worker">The worker we are waiting for.</param>
  /// <param name="timeout">How long we want to wait for.</param>
  /// <returns>Either complete if everything completed or timeout</returns>
  WaitResult WorkerPool::stop_and_wait(Worker& worker, const long long timeout)
  {
    return stop_and_wait({&worker}, timeout );
  }

  /// <summary>
  /// Stop alll the workers and wait for them all to complete.
  /// </summary>
  /// <param name="timeout">The number of ms we want to wait for</param>
  /// <returns>Either timeout or complete if all the workers completed</returns>
  WaitResult WorkerPool::stop_and_wait( const long long timeout)
  {
    // before we stop all workers ... we have
    // to make sure that everybody is started
    start_all_pending_workers();

    // just tell all our workers to stop.
    stop_all_workers();

    // we have to wait for all the futures to complete
    // if they timeout then we cannot stop ourselves.
    if( WaitResult::timeout == wait_for_all_futures_to_complete(timeout) )
    {
      return WaitResult::timeout;
    }

    // now that our futures are complete, (the ones we are aware of)
    // we can call ourselves to stop
    // if we could not complete the futures, then we cannot stop
    return Worker::stop_and_wait(timeout);
  }
  #pragma endregion

  #pragma region Worker
  /// <summary>
  /// Called when a stop request was made
  /// We must stop all the worker and prevent any more from being added.
  /// </summary>
  void WorkerPool::on_worker_stop()
  {
    // before we stop all workers ... we have
    // to make sure that everybody is started
    start_all_pending_workers();

    // send a stop notification to all the workers.
    stop_all_workers();
  }

  /// <summary>
  /// Called when when the pool is starting
  /// </summary>
  /// <returns></returns>
  bool WorkerPool::on_worker_start()
  {
    return start_all_pending_workers();
  }

  /// <summary>
  /// Called at regular intervals
  /// </summary>
  /// <param name="fElapsedTimeMilliseconds"></param>
  /// <returns>False if we want to end the pool or true if we want to continue</returns>
  bool WorkerPool::on_worker_update(const float fElapsedTimeMilliseconds)
  {
    // wait for all the start workers
    wait_for_all_add_futures_pending();

    // assume that none of our workers want to continue
    // ignore the completed workers.
    auto mustContinue = false;

    // update our elasped time
    _fElapsedTimeMilliseconds += fElapsedTimeMilliseconds;

    MYODDWEB_LOCK(_workerAndFuturesLock);
    for (const auto& workerAndFuture : _workerAndFutures)
    {
      // the worker
      const auto worker = workerAndFuture.first;

      // if that worker is completed then we do not care
      // it will be removed at some other poing
      if (worker->completed())
      {
        continue;
      }

      // check if this worker has started
      if (!worker->started())
      {
        // does it wants to start
        if (!worker->worker_start())
        {
          // it does not want to start so it has to be completed.
          // we do not change the mustContinue flag in case
          // another worker wants to continue.
          assert(worker->completed());
          continue;
        }
      }

      // while we are in the quick update loop, we want to check if the future returned false,
      // or if we completed our end workers.
      // if it did, then we need to end it right away rather than waiting for the next loop.
      const auto end = get_end_future_end_state_in_lock(*worker);
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

      const auto update = get_update_future_end_state_in_lock(*worker);
      if(FutureEndState::CompleteFalse == update )
      {
        // the worker returned false, so it wants to end
        worker_end_in_lock( *worker );

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
      if (!update_once_in_lock( *worker, _fElapsedTimeMilliseconds ))
      {
        worker_end_in_lock(*worker);

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
    return mustContinue || has_add_futures_pending();
  }

  /// <summary>
  /// When the worker pool has ended.
  /// </summary>
  void WorkerPool::on_worker_end()
  {
    // wait for all the start workers
    wait_for_all_add_futures_pending();

    {
      MYODDWEB_LOCK(_workerAndFuturesLock);
      for (const auto& workerAndFutures : _workerAndFutures)
      {
        const auto worker = workerAndFutures.first;

        // if our worker is still running then we cannot end the worker
        if (FutureEndState::StillRunning == get_update_future_end_state_in_lock(*worker))
        {
          continue;
        }

        // no need to check if stopped already or not
        // the worker class checks if we can call it.
        worker_end_in_lock(*worker);
      }
    }

    // wait for all futures to complete before ending
    wait_for_all_futures_to_complete(-1);
  }
  #pragma endregion

  #pragma region Private Helpers
  /// <summary>
  /// Start all the workers that have yet to start
  /// We return false if _all_ the workers returned false
  /// Or if we have no worker pending.
  /// </summary>
  /// <returns></returns>
  bool WorkerPool::start_all_pending_workers()
  {
    // wait for all the start workers
    wait_for_all_add_futures_pending();

    // if anything started and returned true
    // by default we assume that nothing started
    auto startedOrRunning = false;

    MYODDWEB_LOCK(_workerAndFuturesLock);
    for (const auto& workerAndFuture : _workerAndFutures)
    {
      const auto worker = workerAndFuture.first;
      if(worker->completed())
      {
        // if it is completed then it is not started
        // or even currently running.
        continue;
      }

      if (worker->started() )
      {
        // if it is started then we have to assume that it is still running
        // so we want to "carry-on" running rather than
        // returning false and giving the false impression that nothing started
        startedOrRunning = true;
        continue;
      }

      if (!worker->worker_start())
      {
        // it does not want to start so it has to be completed.
        assert(worker->completed());
        continue;
      }
      startedOrRunning = true;
    }

    // return if one of them started and/or one of them
    // is still currently running.
    return startedOrRunning;
  }

  /// <summary>
  /// Get the future for a worker while we have the lock
  /// We return null if we do not have one.
  /// </summary>
  /// <param name="worker">The worker we are looking for</param>
  /// <returns>The future</returns>
  WorkerPool::Futures* WorkerPool::get_futures_worker_in_lock(Worker& worker) const
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
  void WorkerPool::stop_all_workers()
  {
    MYODDWEB_LOCK(_workerAndFuturesLock);
    for (const auto& workerAndFutures : _workerAndFutures)
    {
      auto worker = workerAndFutures.first;
      // no need to check if stopped already or not
      // the worker class checks if we can call it.
      worker->stop();
    }
  }

  /// <summary>
  /// Delete the worker thread if the work is complete
  /// So that it can be re-used if needed.
  /// </summary>
  void WorkerPool::delete_worker_thread_if_complete()
  {
    MYODDWEB_LOCK(_threadLock);
    if (!must_stop() && !is(State::complete))
    {
      return;
    }
    delete _thread;
    _thread = nullptr;
    _fElapsedTimeMilliseconds = 0;
    set_state(State::unknown);
  }

  /// <summary>
  /// Safely start the worker thread if needed.
  /// </summary>
  void WorkerPool::start_worker_thread_if_needed()
  {
    delete_worker_thread_if_complete();

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
  int WorkerPool::number_of_incomplete_workers() const
  {
    MYODDWEB_LOCK(_workerAndFuturesLock);
    auto number = 0;
    for (const auto& workerAndFutures : _workerAndFutures)
    {
      const auto worker = workerAndFutures.first;
      if (!worker->completed())
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
  void WorkerPool::add_worker(Worker& worker)
  {
    // if the work is complete then we need to restart it
    delete_worker_thread_if_complete();

    MYODDWEB_LOCK(_workerAndFuturesLock);
    // make sure that this worker does not exist already.
    const auto it = _workerAndFutures.find(&worker);
    if( it != _workerAndFutures.end() )
    {
      return;
    }

    _workerAndFutures[&worker] = nullptr;

    // make sure that the thread is running
    start_worker_thread_if_needed();
  }

  /// <summary>
  /// Check if the worker is one of our workers
  /// </summary>
  /// <param name="worker">The worker we are looking for.</param>
  /// <returns></returns>
  bool WorkerPool::exists(Worker& worker) const
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
  WorkerPool::FutureEndState WorkerPool::get_update_future_end_state_in_lock(Worker& worker) const
  {
    // look for the current future
    auto currentFutures = get_futures_worker_in_lock(worker);
    if (currentFutures == nullptr || !currentFutures->has_update())
    {
      // the future us not running at all.
      return FutureEndState::NotRunning;
    }

    // is it valid?
    if (!currentFutures->is_update_valid())
    {
      // no, it is no longer valid, we need to add one.
      currentFutures->discard_update();
      return FutureEndState::NotRunning;
    }

    // so the future for this worker is still running
    // so we want to get a result for it.
    // wait one more ms to see if it needs to be complete.
    const auto wait = std::chrono::milliseconds(1);
    if (currentFutures->is_update_ready(wait))
    {
      // it is complete! So we can get the result from it.
      const auto result = currentFutures->take_update_result();

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
  WorkerPool::FutureEndState WorkerPool::get_end_future_end_state_in_lock(Worker& worker) const
  {
    // look for the current future
    auto currentFutures = get_futures_worker_in_lock(worker);
    if (currentFutures == nullptr || !currentFutures->has_end())
    {
      // the future us not running at all.
      return FutureEndState::NotRunning;
    }

    // is it valid?
    if (!currentFutures->is_end_valid())
    {
      // no, it is no longer valid, we need to add one.
      currentFutures->discard_end();
      return FutureEndState::NotRunning;
    }

    // so the future for this worker is still running
    // so we want to get a result for it.
    // wait one more ms to see if it needs to be complete.
    const auto wait = std::chrono::milliseconds(1);
    if (currentFutures->is_end_ready(wait))
    {
      // it is complete! So we can get the result from it.
      currentFutures->take_end_result();

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
  void WorkerPool::worker_end_in_lock(Worker& worker)
  {
    // if we are not started we do not want to end
    if (!worker.started())
    {
      return;
    }

    // get the futures
    auto futures = _workerAndFutures[&worker];
    if(futures != nullptr && futures->has_end())
    {
      // we called the end already
      return;
    }

    // get the future we will be calling
    const auto newFuture = new std::future<void>(std::async(std::launch::async, worker_end_task(*this, worker)));

    if( nullptr == futures )
    {
      futures = new Futures(nullptr, newFuture);
    }
    else
    {
      // it should have been cleanned up
      assert(!futures->has_update());

      // and we should not have an end already running.
      assert(!futures->has_end());

      // set the end future
      futures->set_end(newFuture);
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
  bool WorkerPool::update_once_in_lock(Worker& worker, const float fElapsedTimeMilliseconds)
  {
    // get the current future status
    switch( get_update_future_end_state_in_lock( worker) )
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
    const auto newFuture = new std::future<bool>(std::async(std::launch::async, worker_update_once_task(*this, worker, fElapsedTimeMilliseconds)));

    // then update the current values.
    auto futures = _workerAndFutures[&worker];
    if (nullptr == futures)
    {
      futures = new Futures(newFuture, nullptr );
    }
    else
    {
      // it should have been cleanned up
      assert(!futures->has_update());

      // set the update future
      futures->set_update( newFuture );
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
  WaitResult WorkerPool::wait_for_all_futures_to_complete(const std::vector<Worker*>& workers, const long long timeout)
  {
    // wait for the operation to complete.
    const auto wait = Wait::spin_until(all_futures_in_list_complete_predicate(*this, workers), timeout);

    // while we are here, remove all the completed workers.
    remove_all_completed_workers();

    // then return if we completed successfully or not.
    return wait ? WaitResult::complete : WaitResult::timeout;
  }

  /// <summary>
  /// Wait for all the workers that still have a future to complete
  /// We do not care about the result, we simply want them to end.
  /// </summary>
  /// <param name="timeout">How long we are prepared to wait for.</param>
  /// <returns></returns>
  WaitResult WorkerPool::wait_for_all_futures_to_complete( const long long timeout)
  {
    // wait for the operation to complete.
    const auto wait = Wait::spin_until(all_futures_complete_predicate(*this), timeout);

    // while we are here, remove all the completed workers.
    remove_all_completed_workers();

    // then return if we completed successfully or not.
    return wait ? WaitResult::complete : WaitResult::timeout;
  }

  /// <summary>
  /// Remove all the completed workers from the list and free the memories
  /// </summary>
  void WorkerPool::remove_all_completed_workers()
  {
    MYODDWEB_LOCK(_workerAndFuturesLock);
    std::vector<Worker*> workersToRemove;

    // first look for everything we want to remove
    for (const auto& workerAndFuture : _workerAndFutures)
    {
      if( !workerAndFuture.first->completed() )
      {
        continue;
      }
      delete workerAndFuture.second;
      workersToRemove.push_back(workerAndFuture.first);
    }

    // then remove them
    for ( const auto& worker : workersToRemove)
    {
      const auto it = _workerAndFutures.find(worker);
      _workerAndFutures.erase(it);
    }
  }

  /// <summary>
  /// Check if we have any wokers still being added.
  /// </summary>
  /// <returns>If we still have workers.</returns>
  bool WorkerPool::has_add_futures_pending()
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
  void WorkerPool::wait_for_all_add_futures_pending()
  {
    Wait::spin_until(no_add_futures_pending_predicate(*this), -1);

#ifdef _DEBUG
    // make sure that the memory is clear.
    assert(_addFutures.empty());
#endif
  }
  #pragma endregion
}
