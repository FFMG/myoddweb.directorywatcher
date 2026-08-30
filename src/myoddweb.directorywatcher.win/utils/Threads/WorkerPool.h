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
    void add(Worker& worker);

    /// <summary>
    /// Wait for a worker to either complete or timeout
    /// </summary>
    /// <param name="worker">The worker we will be waiting for</param>
    /// <param name="timeout">How long to wait for.</param>
    /// <returns>Either complete or timeout</returns>
    WaitResult wait_for(Worker& worker, long long timeout);

    /// <summary>
    /// Wait for all the workers to either finish or the timeout.
    /// </summary>
    /// <param name="timeout">How long to wait for.</param>
    /// <returns>Either complete or timeout</returns>
    WaitResult wait_for(long long timeout) override;

    /// <summary>
    /// Signal a single worker to stop.
    /// </summary>
    /// <param name="worker"></param>
    void stop_worker(Worker& worker);

    /// <summary>
    /// Stop one or more workers and wait for them to complete.
    /// After stopping them we wait for them all or timeout.
    /// </summary>
    /// <param name="workers">The workers we are waiting for.</param>
    /// <param name="timeout">How long we want to wait for.</param>
    /// <returns>Either complete if everything completed or timeout</returns>
    WaitResult stop_and_wait(const std::vector<Worker*>& workers, long long timeout);

    /// <summary>
    /// Wait for a single engine worker to complete and wait for it to complete.
    /// </summary>
    /// <param name="worker">The worker we are waiting for.</param>
    /// <param name="timeout">How long we want to wait for.</param>
    /// <returns>Either complete if everything completed or timeout</returns>
    WaitResult stop_and_wait(Worker& worker, long long timeout);

    /// <summary>
    /// Stop alll the workers and wait for them all to complete.
    /// </summary>
    /// <param name="timeout">The number of ms we want to wait for</param>
    /// <returns>Either timeout or complete if all the workers completed</returns>
    WaitResult stop_and_wait(long long timeout) override;
    #pragma endregion

  protected:
    #pragma region Worker
    /// <summary>
    /// Called when we want to stop
    /// </summary>
    void on_worker_stop() override;

    /// <summary>
    /// Called when whend the pool is starting
    /// </summary>
    /// <returns></returns>
    bool on_worker_start() override;

    /// <summary>
    /// Called at regular intervals
    /// </summary>
    /// <param name="fElapsedTimeMilliseconds"></param>
    /// <returns>False if we want to end the pool or true if we want to continue</returns>
    bool on_worker_update(float fElapsedTimeMilliseconds) override;

    /// <summary>
    /// When the worker pool has ended.
    /// </summary>
    void on_worker_end() override;
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

    class Futures final
    {
    public:
      explicit Futures(std::future<bool>* update, std::future<void>* end) :
        _update(update),
        _end(end)
      {
      }

      Futures(const Futures&) = delete;
      Futures(Futures&&) = delete;
      Futures& operator=(const Futures&) = delete;
      Futures& operator=(Futures&&) = delete;

      ~Futures()
      {
        free_update();
        free_end();
      }

      void set_update(std::future<bool>* update )
      {
        free_update();
        _update = update;
      }

      void set_end(std::future<void>* end)
      {
        free_end();
        _end = end;
      }

      [[nodiscard]]
      bool has_update() const
      {
        return _update != nullptr;
      }

      [[nodiscard]]
      bool has_end() const
      {
        return _end != nullptr;
      }

      [[nodiscard]]
      bool is_update_valid() const
      {
        return _update != nullptr && _update->valid();
      }

      [[nodiscard]]
      bool is_end_valid() const
      {
        return _end != nullptr && _end->valid();
      }

      [[nodiscard]]
      bool is_update_ready(const std::chrono::milliseconds& wait) const
      {
        return _update->wait_for(wait) == std::future_status::ready;
      }

      [[nodiscard]]
      bool is_end_ready(const std::chrono::milliseconds& wait) const
      {
        return _end->wait_for(wait) == std::future_status::ready;
      }

      /// <summary>
      /// Discard the update future, (it is no longer valid), without consuming a result.
      /// </summary>
      void discard_update()
      {
        delete _update;
        _update = nullptr;
      }

      /// <summary>
      /// Discard the end future, (it is no longer valid), without consuming a result.
      /// </summary>
      void discard_end()
      {
        delete _end;
        _end = nullptr;
      }

      /// <summary>
      /// Consume and return the result of a completed update future, then delete it.
      /// </summary>
      bool take_update_result()
      {
        const auto result = _update->get();
        delete _update;
        _update = nullptr;
        return result;
      }

      /// <summary>
      /// Consume the result of a completed end future, then delete it.
      /// </summary>
      void take_end_result()
      {
        _end->get();
        delete _end;
        _end = nullptr;
      }

    private:
      void free_update()
      {
        if (_update != nullptr && _update->valid())
        {
          _update->get();
        }
        delete _update;
        _update = nullptr;
      }
      void free_end()
      {
        if (_end != nullptr && _end->valid())
        {
          _end->get();
        }
        delete _end;
        _end = nullptr;
      }

      std::future<bool>* _update;
      std::future<void>* _end;
    };

    /// <summary>
    /// Call the protected/friend-only Worker::worker_end() on the given worker.
    /// Exists so our private predicate/task functors, (which are not themselves
    /// friends of Worker), can reach it through us.
    /// </summary>
    /// <param name="worker">The worker we want to end.</param>
    void worker_end_now(Worker& worker) const;

    /// <summary>
    /// Call the protected/friend-only Worker::worker_update_once() on the given worker.
    /// Exists so our private predicate/task functors, (which are not themselves
    /// friends of Worker), can reach it through us.
    /// </summary>
    /// <param name="worker">The worker we want to update.</param>
    /// <param name="fElapsedTimeMilliseconds">The elapsed time to pass on.</param>
    /// <returns>True if we want to continue or false if we want to stop.</returns>
    bool worker_update_once_now(Worker& worker, float fElapsedTimeMilliseconds) const;

    /// <summary>
    /// Task used to add a worker to the pool asynchronously.
    /// </summary>
    struct add_worker_task final
    {
    public:
      add_worker_task(WorkerPool& pool, Worker& worker) :
        _pool(pool),
        _worker(worker)
      {
      }

      void operator()() const
      {
        _pool.add_worker(_worker);
      }

    private:
      WorkerPool& _pool;
      Worker& _worker;
    };

    /// <summary>
    /// Task used to stop a single worker, (if it still exists), from within a std::for_each.
    /// </summary>
    struct stop_worker_task final
    {
    public:
      stop_worker_task(WorkerPool& pool, const long long timeout) :
        _pool(pool),
        _timeout(timeout)
      {
      }

      void operator()(Worker* worker) const
      {
        if (_pool.exists(*worker))
        {
          worker->stop_and_wait(_timeout);
        }
      }

    private:
      WorkerPool& _pool;
      long long _timeout;
    };

    /// <summary>
    /// Task used to call worker_end() on a worker asynchronously.
    /// </summary>
    struct worker_end_task final
    {
    public:
      worker_end_task(const WorkerPool& pool, Worker& worker) :
        _pool(pool),
        _worker(worker)
      {
      }

      void operator()() const
      {
        _pool.worker_end_now(_worker);
      }

    private:
      const WorkerPool& _pool;
      Worker& _worker;
    };

    /// <summary>
    /// Task used to call worker_update_once() on a worker asynchronously.
    /// </summary>
    struct worker_update_once_task final
    {
    public:
      worker_update_once_task(const WorkerPool& pool, Worker& worker, const float fElapsedTimeMilliseconds) :
        _pool(pool),
        _worker(worker),
        _fElapsedTimeMilliseconds(fElapsedTimeMilliseconds)
      {
      }

      bool operator()() const
      {
        return _pool.worker_update_once_now(_worker, _fElapsedTimeMilliseconds);
      }

    private:
      const WorkerPool& _pool;
      Worker& _worker;
      float _fElapsedTimeMilliseconds;
    };

    /// <summary>
    /// Predicate checking if all the futures for a given list of workers are complete.
    /// </summary>
    struct all_futures_in_list_complete_predicate final
    {
    public:
      all_futures_in_list_complete_predicate(WorkerPool& pool, const std::vector<Worker*>& workers) :
        _pool(pool),
        _workers(workers)
      {
      }

      bool operator()() const;

    private:
      WorkerPool& _pool;
      const std::vector<Worker*>& _workers;
    };

    /// <summary>
    /// Predicate checking if all the futures we know about are complete.
    /// </summary>
    struct all_futures_complete_predicate final
    {
    public:
      explicit all_futures_complete_predicate(WorkerPool& pool) :
        _pool(pool)
      {
      }

      bool operator()() const;

    private:
      WorkerPool& _pool;
    };

    /// <summary>
    /// Predicate checking if we no longer have any pending "add" futures.
    /// </summary>
    struct no_add_futures_pending_predicate final
    {
    public:
      explicit no_add_futures_pending_predicate(WorkerPool& pool) :
        _pool(pool)
      {
      }

      bool operator()() const
      {
        return !_pool.has_add_futures_pending();
      }

    private:
      WorkerPool& _pool;
    };

    /// <summary>
    /// Safely add a container to the list.
    /// </summary>
    /// <param name="worker">The container to add.</param>
    void add_worker(Worker& worker);

    /// <summary>
    /// Get the current number of running workers
    /// </summary>
    /// <returns>Num number of workers</returns>
    int number_of_incomplete_workers() const;

    /// <summary>
    /// Safely start the worker thread if needed.
    /// </summary>
    void start_worker_thread_if_needed();

    /// <summary>
    /// Delete the worker thread if the work is complete
    /// So that it can be re-used if needed.
    /// </summary>
    void delete_worker_thread_if_complete();

    /// <summary>
    /// Send a motification to stop all the workers.
    /// </summary>
    void stop_all_workers();

    /// <summary>
    /// Check if the worker is one of our workers
    /// </summary>
    /// <param name="worker"></param>
    /// <returns></returns>
    bool exists(Worker& worker) const;

    /// <summary>
    /// Get the future for a worker while we have the lock
    /// We return null if we do not have one.
    /// </summary>
    /// <param name="worker">The worker we are looking for</param>
    /// <returns>The future</returns>
    Futures* get_futures_worker_in_lock(Worker& worker) const;

    /// <summary>
    /// Update a single worker in a lock, either check the result or create a result
    /// </summary>
    /// <param name="worker"></param>
    /// <param name="fElapsedTimeMilliseconds"></param>
    /// <returns>True if we want to continue or false if we want to stop.</returns>
    bool update_once_in_lock(Worker& worker, float fElapsedTimeMilliseconds);

    /// <summary>
    /// Call the worker end for this worker and create a future for it.
    /// </summary>
    /// <param name="worker"></param>
    void worker_end_in_lock(Worker& worker);

    /// <summary>
    /// Get the state of the future, (complete, running, ...)
    /// We also delete the future if it is no longer needed.
    /// </summary>
    /// <param name="worker"></param>
    /// <returns></returns>
    FutureEndState get_update_future_end_state_in_lock(Worker& worker) const;

    /// <summary>
    /// Get the state of the future, (complete, running, ...)
    /// We also delete the future if it is no longer needed.
    /// </summary>
    /// <param name="worker"></param>
    /// <returns></returns>
    FutureEndState get_end_future_end_state_in_lock(Worker& worker) const;

    /// <summary>
    /// Wait for all the workers that still have a future to complete
    /// We do not care about the result, we simply want them to end.
    /// </summary>
    /// <param name="timeout">How long we are prepared to wait for.</param>
    /// <returns></returns>
    WaitResult wait_for_all_futures_to_complete(long long timeout);

    /// <summary>
    /// Wait for all the futures in a list of workers to complete.
    /// </summary>
    /// <param name="workers">The workers</param>
    /// <param name="timeout">How long we want to wait</param>
    /// <returns></returns>
    WaitResult wait_for_all_futures_to_complete( const std::vector<Worker*>& workers, long long timeout);

    /// <summary>
    /// Remove all the completed workers from the list and free the memories
    /// </summary>
    void remove_all_completed_workers();

    /// <summary>
    /// Check if we still have some pending "Add" futures.
    /// </summary>
    /// <returns></returns>
    bool has_add_futures_pending();

    /// <summary>
    /// Wait for all the add futures to complete.
    /// </summary>
    void wait_for_all_add_futures_pending();

    /// <summary>
    /// Start all the workers that have yet to start
    /// We return false if _all_ the workers returned false
    /// Or if we have no worker pending.
    /// </summary>
    /// <returns></returns>
    bool start_all_pending_workers();
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
    /// All the futures to add workers.
    /// </summary>
    std::vector<std::future<void>*> _addFutures;

    /// <summary>
    /// The lock to make sure that we do not update the list of workers
    /// While the list is being updated
    /// </summary>
    mutable MYODDWEB_MUTEX _workerAndFuturesLock;

    /// <summary>
    /// Lock for our add futures so we can update the values as required.
    /// </summary>
    mutable MYODDWEB_MUTEX _addFuturesLock;

    /// <summary>
    /// Lock to prevent multiple threads from updating our own thread.
    /// </summary>
    mutable MYODDWEB_MUTEX _threadLock;
    #pragma endregion
  };
}
