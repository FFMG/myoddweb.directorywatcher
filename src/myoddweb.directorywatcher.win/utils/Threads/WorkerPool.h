// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <mutex>
#include "Thread.h"

namespace myoddweb:: directorywatcher:: threads
{
  class WorkerPool final : protected Worker
  {
    /**
     * \brief the current thread handle, if we have one.
     */
    Thread* _thread;

    /**
     * \brief how often we want to limit this.
     */
    const long long _throttleElapsedTimeMilliseconds;

    /**
     * \brief the elapsed time since the last time we checked this
     */
    float _elapsedTimeMilliseconds = 0;

    /**
     * \brief the lock for the running workers.
     */
    std::recursive_mutex _lockRunningWorkers;

    /**
     * \brief lock for the the workers that are waiting to start;
     */
    std::recursive_mutex _lockWorkersWaitingToStart;

    /**
     * \brief lock for the workers that are waiting to end
     */
    std::recursive_mutex _lockThreadsWaitingToEnd;

    /**
     * \brief the workers that have yet to be started
     */
    std::vector<Worker*> _workersWaitingToStart;

    /**
     * \brief the workers lock that we are waiting to end.
     */
    std::vector<Thread*> _threadsWaitingToEnd;

    /**
     * \brief all our workers that are currently running.
     */
    std::vector<Worker*> _runningWorkers;

  public:
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool(WorkerPool&&) = delete;
    const WorkerPool& operator=(const WorkerPool&) = delete;
    const WorkerPool& operator=(const WorkerPool&&) = delete;

    WorkerPool(long long throttleElapsedTimeMilliseconds);
    virtual ~WorkerPool();

    /**
     * \brief add a worker to our worers pool.
     * \param worker the worker we are trying to add.
     */
    void Add(Worker& worker);

    /**
     * \brief add multiple workers at once
     * \param workers the workers we are adding.
     */
    void Add( const std::vector<Worker*>& workers );

    /**
     * \brief wait a little bit for a worker to finish
     *        if the worker does not exist we just return that it is complete.
     * \param worker the worker we are waiting for
     * \param timeout the number of ms we want to wait for the worker to complete.
     * \return either timeout of complete if the thread completed.
     */
    WaitResult WaitFor(Worker& worker, long long timeout);

    /**
     * \brief wait for an array of workers to complete.
     * \param workers the workers we are waiting for.
     * \param timeout how long we are waiting for.
     * \return if any of them timed out.
     */
    WaitResult WaitFor( const std::vector<Worker*>& workers, long long timeout);

    /**
     * \brief wait a little bit for all the workers to finish
     *        if the worker does not exist we just return that it is complete.
     * \param timeout the number of ms we want to wait for the workers to complete.
     * \return either timeout of complete if the threads completed.
     */
    WaitResult WaitFor( long long timeout);

    /**
     * \brief non blocking call to instruct the thread to stop.
     */
    void OnStop() override;

    /**
     * \brief stop one of the worker
     * \param worker the worker we are waiting for.
     */
    void Stop( Worker& worker );

    /**
     * \brief stop multiple workers
     * \param workers the workers we are waiting for.
     */
    void Stop( const std::vector<Worker*>& workers);

    /**
     * \brief stop the running workers and wait
     * \param timeout the number of ms we want to wait.
     * \return the result of the wait
     */
    WaitResult StopAndWait(long long timeout) override;

    /**
     * \brief stop multiple workers and wait
     * \param workers the workers we are waiting for.
     * \param timeout the number of ms we want to wait for them.
     * \return the result of the wait
     */
    WaitResult StopAndWait( const std::vector<Worker*>& workers, long long timeout);

    /**
     * \brief stop one of the worker and wait
     * \param worker the worker we are waiting for.
     * \param timeout the number of ms we want to wait.
     */
    WaitResult StopAndWait(Worker& worker, long long timeout);

  protected:
    /**
     * \brief check if we stop or not.
     */
    bool CheckIfMustStop();

    /**
     * \brief called when the worker thread is about to start
     */
    bool OnWorkerStart() override;

    /**
     * \brief Give the worker a chance to do something in the loop
     *        Workers can do _all_ the work at once and simply return false
     *        or if they have a tight look they can return true until they need to come out.
     * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
     * \return true if we want to continue or false if we want to end the thread
     */
    bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override;

    /**
     * \brief Called when the thread pool has been completed, all the workers should have completed here.
     *        We are done with all of them now.
     */
    void OnWorkerEnd() override;

  private:
    /**
     * \brief check if the time has now elapsed.
     * \param givenElapsedTimeMilliseconds the number of ms since the last time we checked.
     * \param actualElapsedTimeMilliseconds the number of ms that has expired since our last check.
     * \return if the time has elapsed and we can continue.
     */
    bool HasElapsed( float givenElapsedTimeMilliseconds, float& actualElapsedTimeMilliseconds);

    /**
     * \brief queue a worker to the end thread
     * \param worker the worker we want to end.
     */
    void QueueWorkerEnd(Worker& worker);

    /**
     * \brief Give the worker a chance to do something in the loop
     *        Workers can do _all_ the work at once and simply return false
     *        or if they have a tight look they can return true until they need to come out.
     * \param worker the worker we are managing.
     * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
     * \return true if we want to continue or false if we want to end the thread
     */
    bool WorkerUpdate(Worker& worker, float fElapsedTimeMilliseconds);

    /**
     * \brief make a thread safe copy of the running workers.
     */
    std::vector<Worker*> CloneRunningWorkers();

    /**
     * \brief start any workers that are pending.
     */
    void ProcessWorkersWaitingToStart();

    /**
     * \brief wait for any workers that are stopping.
     */
    void ProcessThreadsWaitingToEnd();

    /**
     * \brief start any workers and thread that need to be started/removed.
     */
    void ProcessThreadsAndWorkersWaiting();

    /**
     * \brief process workers that has indicated the need to stop.
     * \param workers the workers we are wanting to stop
     */
    void ProcessWorkersWaitingToEnd( const std::vector<Worker*>& workers );

    /**
     * \brief remove a workers from our list of posible waiting workers.
     *        we will obtain the lock to remove those items.
     * \return the list of items removed.
     */
    std::vector<Worker*> RemoveWorkersFromWorkersWaitingToStart();

    /**
     * \brief remove a worker from our list of running workers.
     *        we will obtain the lock to remove this item.
     * \param worker the worker we are wanting to remove
     */
    void RemoveWorkerFromRunningWorkers(const Worker& worker);

    /**
     * \brief remove workers from our list of running workers.
     *        we will obtain the lock to remove this items.
     * \param workers the workers we are wanting to remove
     * \return the list of items removed.
     */
    std::vector<Worker*> RemoveWorkersFromRunningWorkers(const std::vector<Worker*>& workers);

    /**
     * \brief remove all the workers from our list of running workers.
     *        we will obtain the lock to remove this items.
     * \return the list of items removed.
     */
    std::vector<Worker*> RemoveWorkersFromRunningWorkers();

    /**
     * \brief remove a single worker from a collection of workers
     * \param container the collection of workers.
     * \param item the worker we want t remove
     */
    static void RemoveWorker(std::vector<Worker*>& container, const Worker& item);

    /**
     * \brief add workers to a list of workers that are waiting to start.
     * \param workers the worker we want to add.
     */
    void AddToWorkersWaitingToStart( const std::vector<Worker*>& workers);

    /**
     * \brief add this worker to our list of running workers
     * \param worker the worker we are adding
     */
    void AddToRunningWorkers(Worker& worker);

    /**
     * \brief had a worker to the container
     * \param container the container we are adding to
     * \param item the worker we want to add.
     */
    static void AddWorker(std::vector<Worker*>& container, Worker& item);

    /**
     * \brief had a worker to the container
     * \param container the container we are adding to
     * \param items the workers we want to add.
     */
    static void AddWorkers(std::vector<Worker*>& container, const std::vector<Worker*>& items);
  };
}