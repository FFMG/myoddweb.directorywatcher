// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <map>
#include "Thread.h"

namespace myoddweb:: directorywatcher:: threads
{
  class WorkerPool final : public Worker
  {
  public:
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool(WorkerPool&&) = delete;
    WorkerPool& operator=(WorkerPool&&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

    /// <summary>
    /// Called when when the pool is starting
    /// </summary>
    /// <param name="throttleElapsedTimeMilliseconds">How often we want updates to happen</param>
    explicit WorkerPool(long long throttleElapsedTimeMilliseconds);
    virtual ~WorkerPool();

    #pragma region Helpers
    /// <summary>
    /// Add a worker to the pool of workers.
    /// </summary>
    /// <param name="worker">The worker we want to add.</param>
    void Add(Worker& worker);

    /// <summary>
    /// Remove a worker from our list of workers.
    /// </summary>
    /// <param name="worker"></param>
    void Remove(Worker& worker);

    /// <summary>
    /// Wait for a worker to either complete or timeout
    /// </summary>
    /// <param name="worker">The worker we will be waiting for</param>
    /// <param name="timeout">How long to wait for.</param>
    /// <returns>Either complete or timeout</returns>
    WaitResult WaitFor(Worker& worker, long long timeout);

    /// <summary>
    /// Wait for all the workers to either finish or the timeout.
    /// </summary>
    /// <param name="timeout">How long to wait for.</param>
    /// <returns>Either complete or timeout</returns>
    WaitResult WaitFor(long long timeout) override;

    /// <summary>
    /// Signal a single worker to stop.
    /// </summary>
    /// <param name="worker"></param>
    void StopWorker(Worker& worker);

    /// <summary>
    /// Stop one or more workers and wait for them to complete.
    /// After stopping them we wait for them all or timeout.
    /// </summary>
    /// <param name="workers">The workers we are waiting for.</param>
    /// <param name="timeout">How long we want to wait for.</param>
    /// <returns>Either complete if everything completed or timeout</returns>
    WaitResult StopAndWait(const std::vector<Worker*>& workers, long long timeout);

    /// <summary>
    /// Wait for a single engine worker to complete and wait for it to complete.
    /// </summary>
    /// <param name="worker">The worker we are waiting for.</param>
    /// <param name="timeout">How long we want to wait for.</param>
    /// <returns>Either complete if everything completed or timeout</returns>
    WaitResult StopAndWait(Worker& worker, long long timeout);

    /// <summary>
    /// Stop alll the workers and wait for them all to complete.
    /// </summary>
    /// <param name="timeout">The number of ms we want to wait for</param>
    /// <returns>Either timeout or complete if all the workers completed</returns>
    WaitResult StopAndWait(long long timeout) override;
    #pragma endregion

  protected:
    #pragma region Worker
    /// <summary>
    /// Called when we want to stop
    /// </summary>
    void OnWorkerStop() override;

    /// <summary>
    /// Called when whend the pool is starting
    /// </summary>
    /// <returns></returns>
    bool OnWorkerStart() override;

    /// <summary>
    /// Called at regular intervals
    /// </summary>
    /// <param name="fElapsedTimeMilliseconds"></param>
    /// <returns>False if we want to end the pool or true if we want to continue</returns>
    bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override;

    /// <summary>
    /// When the worker pool has ended.
    /// </summary>
    void OnWorkerEnd() override;
    #pragma endregion

  private:
    #pragma region Private Helpers
    enum class FutureEndState
    {
      NotRunning,
      StillRunning,
      CompleteTrue,
      CompleteFalse
    };

    class Futures
    {
    public:
      explicit Futures(std::future<bool>* update, std::future<void>* end) :
        _update(update),
        _end(end)
      {
      }

      ~Futures()
      {
        FreeUpdate();
        FreeEnd();
      }

      void SetUpdate(std::future<bool>* update )
      {
        FreeUpdate();
        _update = update;
      }

      void SetEnd(std::future<void>* end)
      {
        FreeEnd();
        _end = end;
      }

      std::future<bool>* _update;
      std::future<void>* _end;

    private:
      void FreeUpdate()
      {
        if (_update != nullptr && _update->valid())
        {
          _update->get();
        }
        delete _update;
        _update = nullptr;
      }
      void FreeEnd()
      {
        if (_end != nullptr && _end->valid())
        {
          _end->get();
        }
        delete _end;
        _end = nullptr;
      }
    };

    /// <summary>
    /// Safely add a container to the list.
    /// </summary>
    /// <param name="worker">The container to add.</param>
    void AddWorker(Worker& worker);

    /// <summary>
    /// Get the current number of running workers
    /// </summary>
    /// <returns>Num number of workers</returns>
    int NumberOfIncompleteWorkers() const;

    /// <summary>
    /// Safely start the worker thread if needed.
    /// </summary>
    void StartWorkerThreadIfNeeded();

    /// <summary>
    /// Delete the worker thread if the work is complete
    /// So that it can be re-used if needed.
    /// </summary>
    void DeleteWorkerThreadIfComplete();

    /// <summary>
    /// Send a motification to stop all the workers.
    /// </summary>
    void StopAllWorkers();

    /// <summary>
    /// Check if the worker is one of our workers
    /// </summary>
    /// <param name="worker"></param>
    /// <returns></returns>
    bool Exists(Worker& worker) const;

    /// <summary>
    /// Get the future for a worker while we have the lock
    /// We return null if we do not have one.
    /// </summary>
    /// <param name="worker">The worker we are looking for</param>
    /// <returns>The future</returns>
    Futures* GetFuturesWorkerInLock(Worker& worker) const;

    /// <summary>
    /// Update a single worker in a lock, either check the result or create a result
    /// </summary>
    /// <param name="worker"></param>
    /// <param name="fElapsedTimeMilliseconds"></param>
    /// <returns>True if we want to continue or false if we want to stop.</returns>
    bool UpdateOnceInLock(Worker& worker, float fElapsedTimeMilliseconds);

    /// <summary>
    /// Call the worker end for this worker and create a future for it.
    /// </summary>
    /// <param name="worker"></param>
    void WorkerEndInLock(Worker& worker);

    /// <summary>
    /// Get the state of the future, (complete, running, ...)
    /// We also delete the future if it is no longer needed.
    /// </summary>
    /// <param name="worker"></param>
    /// <returns></returns>
    FutureEndState GetUpdateFutureEndStateInLock(Worker& worker) const;

    /// <summary>
    /// Get the state of the future, (complete, running, ...)
    /// We also delete the future if it is no longer needed.
    /// </summary>
    /// <param name="worker"></param>
    /// <returns></returns>
    FutureEndState GetEndFutureEndStateInLock(Worker& worker) const;

    /// <summary>
    /// Wait for all the workers that still have a future to complete
    /// We do not care about the result, we simply want them to end.
    /// </summary>
    /// <param name="timeout">How long we are prepared to wait for.</param>
    /// <returns></returns>
    WaitResult WaitForAllFuturesToComplete(long long timeout);

    /// <summary>
    /// Wait for all the futures in a list of workers to complete.
    /// </summary>
    /// <param name="workers">The workers</param>
    /// <param name="timeout">How long we want to wait</param>
    /// <returns></returns>
    WaitResult WaitForAllFuturesToComplete( std::vector<Worker*> workers, long long timeout);

    /// <summary>
    /// Remove all the completed workers from the list and free the memories
    /// </summary>
    void RemoveAllCompletedWorkers();
    #pragma endregion 

    #pragma region Member Variables
    /// <summary>
    /// How often we want updates to happen.
    /// </summary>
    const float _throttleElapsedTimeMilliseconds;

    /// <summary>
    /// This is our actual ellapsed time in ms.
    /// </summary>
    float _fElapsedTimeMilliseconds;

    /// <summary>
    /// Our worker thread.
    /// </summary>
    Thread* _thread;

    /// <summary>
    /// The workers we are currently looking after.
    /// </summary>
    std::map<Worker*, Futures*> _workerAndFutures;

    /// <summary>
    /// The lock to make sure that we do not update the list of workers
    /// While the list is being updated
    /// </summary>
    mutable MYODDWEB_MUTEX _workerAndFuturesLock;
    #pragma endregion
  };
}