// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "Worker.h"
#include "../../monitors/Base.h"
#include "../Instrumentor.h"
#include "../Lock.h"
#include "../Logger.h"
#include "../LogLevel.h"
#include "../Wait.h"
#include "WorkerId.h"

namespace myoddweb::directorywatcher::threads
{
  namespace
  {
    /**
     * \brief predicate used while waiting for this worker to complete.
     */
    struct worker_completed_predicate final
    {
      explicit worker_completed_predicate(const Worker& worker) :
        _worker(worker)
      {
      }

      bool operator()() const
      {
        return _worker.completed();
      }

    private:
      const Worker& _worker;
    };
  }

  Worker::Worker() : Worker( WorkerId::next_id() )
  {
  }

  /// <summary>
  /// The base class can give us an id.
  /// </summary>
  /// <param name="id"></param>
  /// <returns></returns>
  Worker::Worker( const long long id ) :
    _state(State::unknown),
    _id(id)
  {
    // set he current time point
    _timePoint1 = std::chrono::system_clock::now();
    _timePoint2 = _timePoint1;
  }

  Worker::~Worker()
  {
    try
    {
      // the derived class did not complete this operation
      // you should call stop all before deleting
      // we are in the destructor so we can no longer handle this here.
      if (!is(State::complete))
      {
        Logger::log(id(), LogLevel::Panic, L"One of the worker was not completed by the base class!" );
      }
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' trying to complete all operations!", e.what());
    }
  }

  /// <summary>
  /// Get the Id of this worker.
  /// </summary>
  /// <returns></returns>
  const long long& Worker::id() const
  {
    return _id;
  }

  /**
   * \brief Check if the current state is the one we are after given one
   * \param state the state we want to check for.
   * \return if the state is the one we are checking
   */
  bool Worker::is(const State& state) const
  {
    return _state == state;
  }

  /// <summary>
  /// Update the state from one value to anothers.
  /// </summary>
  /// <param name="state">The new value</param>
  void Worker::set_state(const State& state)
  {
    _state = state;
  }

  /**
   * \brief if the thread has completed or not.
   * \return if the thread is still running.
   */
  [[nodiscard]]
  bool Worker::completed() const
  {
    return is(State::complete );
  }

  /**
   * \brief If the worker has started or not.
   * \return if the worker is still running.
   */
  [[nodiscard]]
  bool Worker::started() const
  {
    return !is(State::unknown) && !completed();
  }

  /**
    * \brief If the worker has been told to stop or not.
    * \return if the worker must stop.
    */
  [[nodiscard]]
  bool Worker::must_stop() const
  {
    return is(State::stopped) || is(State::stopping ) || is(State::complete);
  }

  /**
   * \brief non blocking call to instruct the thread to stop.
   */
  void Worker::stop()
  {
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lockState);
    stop_in_lock();
  }

  void Worker::stop_in_lock()
  {
    MYODDWEB_PROFILE_FUNCTION();
    // if the state is unknown it means we never even started
    // there is nothing for us to do here.
    if (is(State::unknown))
    {
      // we are done
      set_state( State::complete );
      return;
    }

    // was it called already?
    // or are we trying to cal it after we are all done?
    if( is(State::stopped) || is(State::complete ))
    {
      return;
    }

    // we are stopping
    set_state( State::stopping );

    // call the derived function
    on_worker_stop();

    // we are done
    set_state( State::stopped );
  }

  /// <summary>
  /// The one and only function that run the complete thread.
  /// </summary>
  void Worker::execute()
  {
    Logger::log(id(), LogLevel::Debug, L"Worker is Starting");
    // start the thread, if it returns false
    // then we will get out.
    if (!worker_start())
    {
      Logger::log(id(), LogLevel::Debug, L"Worker did not want to Start");
      return;
    }

    Logger::log(id(), LogLevel::Debug, L"Worker is Running");

    // run the code
    worker_run();

    Logger::log(id(), LogLevel::Debug, L"Worker is Ending");

    // the thread has ended.
    worker_end();

    Logger::log(id(), LogLevel::Debug, L"Worker is has Ended");
  }

  /// <summary>
  /// Wait for the worker to finish or timeout.
  /// </summary>
  /// <param name="timeout">How long to wait for.</param>
  /// <returns>Either complete or timeout</returns>
  WaitResult Worker::wait_for(const long long timeout)
  {
    // just spin for a while and get out if we complete.
    if (Wait::spin_until(worker_completed_predicate(*this), timeout))
    {
      return WaitResult::complete;
    }
    return WaitResult::timeout;
  }

  /**
   * \brief stop the running thread and wait
   * \param timeout how long we want to wait
   * \return if the worker completed or if we timeed out.
   */
  WaitResult Worker::stop_and_wait(const long long timeout)
  {
    try
    {
      switch (_state)
      {
      case State::unknown:
      case State::complete:
        // we have not really started
        return WaitResult::complete;

      case State::starting:
      case State::started:
      case State::stopping:
      case State::stopped:
        // we have to wait for it to complete.
        break;

      default:
        throw std::exception("Unknown state!");
      }

      // stop it, (maybe again)
      stop();

      // then wait however long we need to.
      return wait_for(timeout);
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' trying to stop and wait!", e.what());

      return WaitResult::timeout;
    }
  }

  /**
   * \brief call the update cycle once only, if we return false the it will be the last one
   * \param fElapsedTimeMilliseconds the number of ms since the last call.
   * \return true if we want to continue, false otherwise.
   */
  bool Worker::worker_update_once(const float fElapsedTimeMilliseconds)
  {
    if (is(State::stopped) || is(State::complete))
    {
      // we have either stopped or the state it complete
      // we have to break out of the loop.
      return false;
    }

    // if we are busy stopping ... but not stoped, we do not want to break
    // out of the loop just yet as we are still busy
    // but we do not want to call update anymore.
    if (is(State::stopping))
    {
      return true;
    }

    // call the update now.
    // if it returns false we will break out of the update look.
    return on_worker_update(fElapsedTimeMilliseconds);
  }

  /**
   * \brief calculate the elapsed time since the last time this call was made
   * \return float the elapsed time in milliseconds.
   */
  float Worker::calculate_elapsed_time_milliseconds()
  {
    // update and calculate the elapsed time.
    _timePoint2 = std::chrono::system_clock::now();
    const std::chrono::duration<float, std::milli> elapsedTime = _timePoint2 - _timePoint1;
    _timePoint1 = _timePoint2;
    return elapsedTime.count();
  }

  /**
   * \brief called when the thread is starting
   *        this should not block anything
   */
  bool Worker::worker_start()
  {
    // grab the lock not because we are doing anything, but because _we_ might be in the middle of an update
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lockState);
    try
    {
      // we are starting
      set_state(State::starting);

      if (!on_worker_start())
      {
        // we could not even start, so we are stopped.
        set_state( State::complete );
        return false;
      }

      // the thread has started work.
      // we could argue that this flag should be set
      // after `on_worker_start()` but this is technically all part of the same thread.
      set_state( State::started );

      // we are done
      return true;
    }
    catch (...)
    {
      save_current_exception();
      return false;
    }
  }

  /**
   * \brief the main body of the thread runner
   *        this function will run until the worker wants to exist.
   */
  void Worker::worker_run()
  {
    try
    {
      // loop forever until we have to get out.
      for (;;)
      {
        try
        {
          // make sure that we yield to other thread
          // from time to time.
          MYODDWEB_YIELD();

          // grab the lock not because we are doing anything, but because _we_ might be in the middle of an update
          MYODDWEB_LOCK(_lockState);

          // update once only.
          if( !worker_update_once(calculate_elapsed_time_milliseconds()) )
          {
            break;
          }
        }
        catch( ... )
        {
          save_current_exception();
        }
      }
    }
    catch (...)
    {
      save_current_exception();
    }
  }

  /**
   * \brief called when the thread is ending
       *        this should not block anything
   */
  void Worker::worker_end()
  {
    // grab the lock not because we are doing anything, but because _we_ might be in the middle of an update
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lockState);

    try
    {
      // if we are complete already then we are done
      if (is(State::complete))
      {
        return;
      }

      // whatever happens we can call the 'stop' call now
      // if that call was made earlier, (to cause us to break out of the Update loop), it will be ignored
      // depending on the state, so it does not harm to call it again
      stop_in_lock();

      // the worker has now stopped, so we can call the blocking call
      // to give the worker a chance to finish/dispose everything that needs to be disposed.
      on_worker_end();
    }
    catch (...)
    {
      save_current_exception();
    }

    // whatever happens, we have now completed
    // nothing else can happen after this.
    set_state( State::complete );
  }

  /**
   * \brief save the current exception
   */
  void Worker::save_current_exception() const
  {
    try {
      const auto ptr = std::current_exception();
      if (ptr)
      {
        std::rethrow_exception(ptr);
      }
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs'", e.what() );
    }
  }
}
