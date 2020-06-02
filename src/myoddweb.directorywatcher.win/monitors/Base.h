// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

#include "../utils/Wait.h"

// create a variable
#define MYODDWEB_VAR(z) line##z##var
#define MYODDWEB_DEC(x) MYODDWEB_VAR(x)

#define MYODDWEB_YIELD()      \
{                             \
  Wait::YieldOnce();          \
}

#ifdef _DEBUG
  #include <iostream>
  #define MYODDWEB_OUT( what ) std::cout << (what);
#else
  #define MYODDWEB_OUT
#endif

namespace myoddweb:: directorywatcher
{
  /**
   * \brief how often we want the worker pool throttle to do monitor updates
   *        the worker loop is once ever 1ms or, (sometimes less), so it does not make sense to have it too often.
   *        the smaller that amount of time, the bigger the cpu usage
   *        for example, an update ever 10ms would give 100 updates per seconds ... 
   */
  const auto MYODDWEB_WORKERPOOL_THROTTLE = 10L;

  /**
   * \brief The min number of Milliseconds we want to wait for an IO signal.
   *        If this number is too low then we will use more CPU.
   */
  constexpr auto MYODDWEB_MIN_THREAD_SLEEP = 50L;

  /**
   * \brief The min number of Milliseconds we want to wait for an IO signal.
   *        because the thread pool managed more than one thread this number can be lower
   */
  constexpr auto MYODDWEB_MIN_THREADPOOL_SLEEP = 5;
  
  /** 
   * \brief similar to the call above, but this puts our thread to sleep
   *        completely, giving other threads a change to process data.
   *        If this number is too low then we will use more CPU.
   */
  constexpr auto MYODDWEB_CPU_THREAD_SLEEP = 100L;
  
  /**
   * \brief When a handle becomes invalid, (the watched folder was deleted)
   *        How often do we want to re-check the folder and, if valid
   *        How often do we want to try and re-open it.
   */
  constexpr auto  MYODDWEB_INVALID_HANDLE_SLEEP = 5000L;

  /**
   * \brief The maximum number of subpath we want to allow in multiple windows monitor.
   *        If the number is too large the number of running threads will cause issues.
   */
  constexpr auto MYODDWEB_MAX_NUMBER_OF_SUBPATH = 64L;

  /**
   * \brief how long we want to wait for the various IOs to complete before
   *        we stop the watcher.
   */
  constexpr auto MYODDWEB_WAITFOR_IO_COMPLETION = 1000;

  /**
   * \brief how long was want to wait for the aborted windows to complete.
   */
  constexpr auto MYODDWEB_WAITFOR_OPERATION_ABORTED_COMPLETION = 15000;

  /**
   * \brief how long we want to wait for the various threads to complete.
   *        before we stop waiting and move on
   *        This value should not be too small otherwise have dangling threads.
   */
  constexpr auto MYODDWEB_WAITFOR_WORKER_COMPLETION = 5000;

  /**
   * \brief the type of worker we are using
   *        1 = std::thread
   *        2 = std::future
   */
  constexpr auto MYODDWEB_WORKER_TYPE = 1;

  /**
   * \brief how often we want to check for 'over-full' containers.
   *        we will delete events that are older than this number.
   */
  constexpr auto MYODDWEB_MAX_EVENT_AGE = 5000;
}
