// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "Worker.h"
#include <thread>
#include "../../monitors/Base.h"
#include "../Instrumentor.h"
#include "../Wait.h"

namespace myoddweb::directorywatcher::threads
{
  Worker::Worker() :
    _state( State::unknown )
  {
    // set he current time point
    _timePoint1 = std::chrono::system_clock::now();
    _timePoint2 = _timePoint1;
  }

  Worker::~Worker()
  {
    Worker::CompleteAllOperations();
  }

  /**
 * \brief make sure that all operations are safely completed.
 *        does not throw an exception
 */
  void Worker::CompleteAllOperations() noexcept
  {
    try
    {
      if (_state != State::complete) 
      {
        Worker::StopAndWait(-1);
      }
    }
    catch (...)
    {

    }
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
    return Is(State::started);
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

    // was it called already?
    // or are we trying to cal it after we are all done?
    if(Is(State::stopped) || Is(State::stopping) || Is(State::complete ))
    {
      return;
    }

    // we are stopping
    _state = State::stopping;

    // call the derived function
    OnStop();

    // we are done
    _state = State::stopped;
  }

  /**
   * \brief Function called to run the thread.
   */
  void Worker::Start()
  {
    // start the thread, if it returns false
    // then we will get out.
    if (!WorkerStart())
    {
      return;
    }

    // run the code
    WorkerRun();

    // the thread has ended.
    WorkerEnd();
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
      MYODDWEB_YIELD();

      // then wait for it to complete.
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

      // wait for it
      if (false == Wait::SpinUntil([&]
        {
          return Completed();
        },
        timeout))
      {
        return WaitResult::timeout;
      }

      // done
      return WaitResult::complete;
    }
    catch( ... )
    {
      return WaitResult::timeout;
    }
  }

  /**
   * \brief called when the thread is starting
   *        this should not block anything
   */
  bool Worker::WorkerStart()
  {
    // we are stopping
    _state = State::starting;
    try
    {
      if (!OnWorkerStart())
      {
        // we could not even start, so we are stopped.
        _state = State::complete;
        return false;
      }

      // the thread has started work.
      // we could argue that this flag should be set
      // after `OnWorkerStart()` but this is technically all part of the same thread.
      _state = State::started;

      // we are done
      return true;
    }
    catch (...)
    {
      _exceptions.push_back(std::current_exception());
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

          // update once only.
          if( !WorkerUpdateOnce(CalculateElapsedTimeMilliseconds()) )
          {
            break;
          }
        }
        catch( ... )
        {
          _exceptions.push_back(std::current_exception());
        }
      }
    }
    catch (...)
    {
      _exceptions.push_back(std::current_exception());
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
    return OnWorkerUpdate( fElapsedTimeMilliseconds);
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
   * \brief called when the thread is ending
       *        this should not block anything
   */
  void Worker::WorkerEnd()
  {
    try
    {
      // whatever happens we can call the 'stop' call now
      // if that call was made earlier, (to cause us to break out of the Update loop), it will be ignored
      // depending on the state, so it does not harm to call it again
      Stop();

      // because of where we are, we have to wait for the work to complete.
      // we cannot do the workend until the state is actually stopped.
      if (!Is(State::stopped))
      {
        Wait::SpinUntil([=]
        {
          return _state == State::stopped;
        }, 
        -1);
      }

      // the worker has now stopped, so we can call the blocking call
      // to give the worker a chance to finish/dispose everything that needs to be disposed.
      OnWorkerEnd();
    }
    catch (...)
    {
      _exceptions.push_back(std::current_exception());
    }

    // whatever happens, we have now completed
    // nothing else can happen after this.
    _state = State::complete;
  }
}
