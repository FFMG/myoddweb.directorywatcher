// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <chrono>
#include <mutex>

#include "../../monitors/Base.h"
#include "WaitResult.h"

namespace myoddweb:: directorywatcher:: threads
{
  class WorkerPool;
  class Worker
  {
  public:
    enum class State
    {
      unknown,
      starting,
      started,
      stopping,
      stopped,
      complete
    };

  private:
    /// <summary>
    /// The current state of the worker.
    /// </summary>
    State _state;
    
    /**
     * \brief The timers used to calculate the elapsed time.
     */
    std::chrono::time_point<std::chrono::system_clock> _timePoint1, _timePoint2;

    /**
     * \brief This lock is used when we are inside the main update function
     *        This will prevent worker end.
     */
    MYODDWEB_MUTEX _lockState;

  public:
    Worker(const Worker&) = delete;
    Worker(Worker&&) = delete;
    Worker& operator=(Worker&&) = delete;
    Worker& operator=(const Worker&) = delete;

    explicit Worker();
    virtual ~Worker();

    /**
     * \brief if the thread has completed or not.
     * \return if the thread is still running.
     */
    [[nodiscard]]
    bool Completed() const;

    /**
     * \brief If the worker has started or not.
     * \return if the worker is still running.
     */
    [[nodiscard]]
    bool Started() const;

    /**
     * \brief If the worker has been told to stop or not.
     * \return if the worker must stop.
     */
    [[nodiscard]]
    bool MustStop() const;

    /// <summary>
    /// The one and only function that run the complete thread.
    /// </summary>
    void Execute();

    /**
     * \brief non blocking call to instruct the thread to stop.
     */
    void Stop();

    /// <summary>
    /// Stop the execution and wait for it to complete.
    /// </summary>
    /// <param name="timeout"></param>
    /// <returns></returns>
    virtual WaitResult StopAndWait( long long timeout );

    /// <summary>
    /// Wait for the worker to finish or timeout.
    /// </summary>
    /// <param name="timeout">How long to wait for.</param>
    /// <returns>Either complete or timeout</returns>
    virtual WaitResult WaitFor(long long timeout);
  private:
    /**
     * \brief called when the thread is starting
     *        this should not block anything
     */
    bool WorkerStart();

    /**
     * \brief the main body of the thread runner
     *        this function will run until the end of the thread.
     */
    void WorkerRun();

    /**
     * \brief called when the thread is ending
     *        this should not block anything
     */
    void WorkerEnd();

    /**
     * \brief calculate the elapsed time since the last time this call was made
     * \return float the elapsed time in milliseconds.
     */
    float CalculateElapsedTimeMilliseconds();

    void StopInLock();
  protected:
    friend WorkerPool;

    /**
     * \brief save the current exception
     */
    void SaveCurrentException() const;

    /**
     * \brief call the update cycle once only, if we return false the it will be the last one
     * \param fElapsedTimeMilliseconds the number of ms since the last call.
     * \return true if we want to continue, false otherwise.
     */
    bool WorkerUpdateOnce(float fElapsedTimeMilliseconds);

    /**
     * \brief Check if the current state is the one we are after given one
     * \param state the state we want to check for.
     * \return if the state is the one we are checking
     */
    [[nodiscard]]
    bool Is(const State& state) const;

    /// <summary>
    /// Update the state from one value to anothers.
    /// </summary>
    /// <param name="state">The new value</param>
    void SetState(const State& state);

    /**
     * \brief called when the worker is ready to start
     *        return false if you do not wish to start the worker.
     */
    virtual bool OnWorkerStart() = 0;

    /**
     * \brief Give the worker a chance to do something in the loop
     *        Workers can do _all_ the work at once and simply return false
     *        or if they have a tight look they can return true until they need to come out.
     * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
     * \return true if we want to continue or false if we want to end the thread
     */
    virtual bool OnWorkerUpdate(float fElapsedTimeMilliseconds) = 0;

    /**
     * \brief called when the worker has completed
     *        this is to allow our workers a chance to dispose of data
     *        
     */
    virtual void OnWorkerEnd()
    {
    }

    /**
     * \brief called when stop is called.
     *        this is to allow our derived workers to stop
     */
    virtual void OnWorkerStop() = 0;

    /**
     * \brief make sure that all operations are safely completed.
     *        does not throw an exception
     */
    virtual void CompleteAllOperations() noexcept;
  };
}
