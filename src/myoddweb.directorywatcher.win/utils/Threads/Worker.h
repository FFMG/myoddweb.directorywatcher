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

    /// <summary>
    /// The id of this worker.
    /// </summary>
    long long _id;

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
    bool completed() const;

    /**
     * \brief If the worker has started or not.
     * \return if the worker is still running.
     */
    [[nodiscard]]
    bool started() const;

    /**
     * \brief If the worker has been told to stop or not.
     * \return if the worker must stop.
     */
    [[nodiscard]]
    bool must_stop() const;

    /// <summary>
    /// The one and only function that run the complete thread.
    /// </summary>
    void execute();

    /**
     * \brief non blocking call to instruct the thread to stop.
     */
    void stop();

    /// <summary>
    /// Get the Id of this worker.
    /// </summary>
    /// <returns></returns>
    [[nodiscard]]
    const long long& id() const;

    /// <summary>
    /// Stop the execution and wait for it to complete.
    /// </summary>
    /// <param name="timeout"></param>
    /// <returns></returns>
    virtual WaitResult stop_and_wait( long long timeout );

    /// <summary>
    /// Wait for the worker to finish or timeout.
    /// </summary>
    /// <param name="timeout">How long to wait for.</param>
    /// <returns>Either complete or timeout</returns>
    virtual WaitResult wait_for(long long timeout);

  protected:
    friend WorkerPool;

    /// <summary>
    /// The base class can give us an id.
    /// </summary>
    /// <param name="id"></param>
    /// <returns></returns>
    Worker(long long id);

    /**
     * \brief called when the thread is starting
     *        this should not block anything
     */
    bool worker_start();

    /**
     * \brief the main body of the thread runner
     *        this function will run until the end of the thread.
     */
    void worker_run();

    /**
     * \brief called when the thread is ending
     *        this should not block anything
     */
    void worker_end();

  private:
    /**
     * \brief calculate the elapsed time since the last time this call was made
     * \return float the elapsed time in milliseconds.
     */
    float calculate_elapsed_time_milliseconds();

    /// <summary>
    /// Stop everything  while we have the lock
    /// </summary>
    void stop_in_lock();

  protected:
    /**
     * \brief save the current exception
     */
    void save_current_exception() const;

    /**
     * \brief call the update cycle once only, if we return false the it will be the last one
     * \param fElapsedTimeMilliseconds the number of ms since the last call.
     * \return true if we want to continue, false otherwise.
     */
    bool worker_update_once(float fElapsedTimeMilliseconds);

    /**
     * \brief Check if the current state is the one we are after given one
     * \param state the state we want to check for.
     * \return if the state is the one we are checking
     */
    [[nodiscard]]
    bool is(const State& state) const;

    /// <summary>
    /// Update the state from one value to anothers.
    /// </summary>
    /// <param name="state">The new value</param>
    void set_state(const State& state);

    /// <summary>
    /// called when the worker is ready to start
    /// </summary>
    /// <returns>false if you do not wish to start the worker.</returns>
    virtual bool on_worker_start() = 0;

    /// <summary>
    /// Give the worker a chance to do something in the loop
    /// Workers can do _all_ the work at once and simply return false
    /// or if they have a tight look they can return true until they need to come out.
    /// </summary>
    /// <param name="fElapsedTimeMilliseconds">the amount of time since the last time we made this call.</param>
    /// <returns> true if we want to continue or false if we want to end the thread.</returns>
    virtual bool on_worker_update(float fElapsedTimeMilliseconds) = 0;

    /**
     * \brief called when the worker has completed
     *        this is to allow our workers a chance to dispose of data
     *
     */
    virtual void on_worker_end() = 0;

    /**
     * \brief called when stop is called.
     *        this is to allow our derived workers to stop
     */
    virtual void on_worker_stop() = 0;
  };
}
