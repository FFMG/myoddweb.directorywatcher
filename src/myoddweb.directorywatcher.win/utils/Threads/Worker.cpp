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
  Worker::Worker() : Worker( WorkerId::NextId() )
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
      if (!Is(State::complete))
      {
        Logger::Log(Id(), LogLevel::Panic, L"One of the worker was not completed by the base class!" );
      }
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' trying to complete all operations!", e.what());
    }
  }

  /// <summary>
  /// Get the Id of this worker.
  /// </summary>
  /// <returns></returns>
  const long long& Worker::Id() const
  {
    return _id;
  }

  /**
   * \brief Check if the current state is the one we are after given one
   * \param state the state we want to check for.
   * \return if the state is the one we are checking
   */
  bool Worker::Is(const State& state) const
  {
    return _state == state;
  }

  /// <summary>
  /// Update the state from one value to anothers.
  /// </summary>
  /// <param name="state">The new value</param>
  void Worker::SetState(const State& state)
  {
    _state = state;
  }

  /**
   * \brief if the thread has completed or not.
   * \return if the thread is still running.
   */
  [[nodiscard]]
  bool Worker::Completed() const
  {
    return Is(State::complete );
  }

  /**
   * \brief If the worker has started or not.
   * \return if the worker is still running.
   */
  [[nodiscard]]
  bool Worker::Started() const
  {
    return !Is(State::unknown) && !Completed();
  }

  /**
    * \brief If the worker has been told to stop or not.
    * \return if the worker must stop.
    */
  [[nodiscard]]
  bool Worker::MustStop() const
  {
    return Is(State::stopped) || Is(State::stopping ) || Is(State::complete);
  }

  /**
   * \brief non blocking call to instruct the thread to stop.
   */
  void Worker::Stop()
  {
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lockState);
    StopInLock();
  }

  void Worker::StopInLock()
  {
    MYODDWEB_PROFILE_FUNCTION();
    // if the state is unknown it means we never even started
    // there is nothing for us to do here.
    if (Is(State::unknown))
    {
      // we are done
      SetState( State::complete );
      return;
    }

    // was it called already?
    // or are we trying to cal it after we are all done?
    if( Is(State::stopped) || Is(State::complete ))
    {
      return;
    }

    // we are stopping
    SetState( State::stopping );

    // call the derived function
    OnWorkerStop();

    // we are done
    SetState( State::stopped );
  }

  /// <summary>
  /// The one and only function that run the complete thread.
  /// </summary>
  void Worker::Execute()
  {
    Logger::Log(Id(), LogLevel::Debug, L"Worker is Starting");
    // start the thread, if it returns false
    // then we will get out.
    if (!WorkerStart())
    {
      Logger::Log(Id(), LogLevel::Debug, L"Worker did not want to Start");
      return;
    }

    Logger::Log(Id(), LogLevel::Debug, L"Worker is Running");

    // run the code
    WorkerRun();

    Logger::Log(Id(), LogLevel::Debug, L"Worker is Ending");

    // the thread has ended.
    WorkerEnd();

    Logger::Log(Id(), LogLevel::Debug, L"Worker is has Ended");
  }

  /// <summary>
  /// Wait for the worker to finish or timeout.
  /// </summary>
  /// <param name="timeout">How long to wait for.</param>
  /// <returns>Either complete or timeout</returns>
  WaitResult Worker::WaitFor(const long long timeout)
  {
    // just spin for a while and get out if we complete.
    if (Wait::SpinUntil([this]
      {
        return Completed();
      }, timeout))
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
  WaitResult Worker::StopAndWait(const long long timeout)
  {
    try
    {
      switch (_state)
      {
      case State::unknown:
      case State::starting:
      case State::complete:
        // we have not really started
        return WaitResult::complete;

      case State::started:
      case State::stopping:
      case State::stopped:
        // we have to wait for it to complete.
        break;

      default:
        throw std::exception("Unknown state!");
      }
 
      // stop it, (maybe again)
      Stop();

      // then wait however long we need to.
      return WaitFor(timeout);
    }
    catch (std::exception& e)
    {
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' trying to stop and wait!", e.what());
    
      return WaitResult::timeout;
    }
  }

  /**
   * \brief call the update cycle once only, if we return false the it will be the last one
   * \param fElapsedTimeMilliseconds the number of ms since the last call.
   * \return true if we want to continue, false otherwise.
   */
  bool Worker::WorkerUpdateOnce(const float fElapsedTimeMilliseconds)
  {
    if (Is(State::stopped) || Is(State::complete))
    {
      // we have either stopped or the state it complete
      // we have to break out of the loop.
      return false;
    }

    // if we are busy stopping ... but not stoped, we do not want to break
    // out of the loop just yet as we are still busy
    // but we do not want to call update anymore.
    if (Is(State::stopping))
    {
      return true;
    }

    // call the update now.
    // if it returns false we will break out of the update look.
    return OnWorkerUpdate(fElapsedTimeMilliseconds);
  }

  /**
   * \brief calculate the elapsed time since the last time this call was made
   * \return float the elapsed time in milliseconds.
   */
  float Worker::CalculateElapsedTimeMilliseconds()
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
  bool Worker::WorkerStart()
  {
    // grab the lock not because we are doing anything, but because _we_ might be in the middle of an update
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lockState);
    try
    {
      // we are starting
      SetState(State::starting);

      if (!OnWorkerStart())
      {
        // we could not even start, so we are stopped.
        SetState( State::complete );
        return false;
      }

      // the thread has started work.
      // we could argue that this flag should be set
      // after `OnWorkerStart()` but this is technically all part of the same thread.
      SetState( State::started );

      // we are done
      return true;
    }
    catch (...)
    {
      SaveCurrentException();
      return false;
    }
  }

  /**
   * \brief the main body of the thread runner
   *        this function will run until the worker wants to exist.
   */
  void Worker::WorkerRun()
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
          if( !WorkerUpdateOnce(CalculateElapsedTimeMilliseconds()) )
          {
            break;
          }
        }
        catch( ... )
        {
          SaveCurrentException();
        }
      }
    }
    catch (...)
    {
      SaveCurrentException();
    }
  }

  /**
   * \brief called when the thread is ending
       *        this should not block anything
   */
  void Worker::WorkerEnd()
  {
    // grab the lock not because we are doing anything, but because _we_ might be in the middle of an update
    MYODDWEB_PROFILE_FUNCTION();
    MYODDWEB_LOCK(_lockState);

    try
    {
      // if we are complete already then we are done
      if (Is(State::complete))
      {
        return;
      }

      // whatever happens we can call the 'stop' call now
      // if that call was made earlier, (to cause us to break out of the Update loop), it will be ignored
      // depending on the state, so it does not harm to call it again
      StopInLock();

      // the worker has now stopped, so we can call the blocking call
      // to give the worker a chance to finish/dispose everything that needs to be disposed.
      OnWorkerEnd();
    }
    catch (...)
    {
      SaveCurrentException();
    }

    // whatever happens, we have now completed
    // nothing else can happen after this.
    SetState( State::complete );
  }

  /**
   * \brief save the current exception
   */
  void Worker::SaveCurrentException() const
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
      Logger::Log(LogLevel::Error, L"Caught exception '%hs'", e.what() );
    }
  }
}
