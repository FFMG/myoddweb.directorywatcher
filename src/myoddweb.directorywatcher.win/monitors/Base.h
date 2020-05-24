// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#if defined(_WIN32)
  #include <Windows.h>

  // sleep a bit, we must be alertable so we can pass/receive messages.
  #define MYODDWEB_ALERTABLE_SLEEP( ms) \
  {                                     \
    ::SleepEx(ms, true);                \
  }
#else
  #include <chrono>
  #include <thread>

  #define MYODDWEB_ALERTABLE_SLEEP( ms)                         \
  {                                                             \
    std::this_thread::sleep_for(std::chrono::milliseconds(ms)); \
  }
#endif

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief The min number of Milliseconds we want to wait for an IO signal.
     *        If this number is too low then we will use more CPU.
     */
    constexpr auto MYODDWEB_MIN_THREAD_SLEEP = 50L;

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
     * \brief The optimal number of threads per monitors.
     *        This nnumber is difficult to match in many cases as not all subfolders can be watched
     *        And/or there might not even be that many folders to watch.
     */
    constexpr auto MYODDWEB_OPTIMAL_NUMBER_OF_THREADS_PER_MONITOR = 200L;

    /**
     * \brief how long we want to wait for the various IOs to complete before
     *        we stop the watcher.
     */
    constexpr auto MYODDWEB_WAITFOR_IO_COMPLETION = 1000;

    /**
     * \brief how long we want to wait for the various threads to complete.
     *        before we stop waiting and move on
     *        This value should not be too small otherwise have dangling threads.
     */
    constexpr auto MYODDWEB_WAITFOR_WORKER_COMPLETION = 5000;

    /**
     * \brief if this macro is defined we will use std::futures
     *        otherwise we will use std::thread
     */
    //#define MYODDWEB_USE_FUTURE 1
  }
}
