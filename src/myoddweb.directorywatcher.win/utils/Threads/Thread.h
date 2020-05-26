// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <thread>
#include <functional>

#include "WaitResult.h"
#include "Worker.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace threads
    {
      typedef std::function<void()> TCallback;

      class Thread final
      {
        Worker* _localWorker;
        Worker* _parentWorker;

#ifdef MYODDWEB_USE_FUTURE
        std::future<void>* _future;
#else
        std::thread* _thread;
#endif
        void CreateWorker(Worker* worker);

        /**
         * \brief the common constructor, private as used to set default values.
         */
        Thread();

      public:
        Thread(const Thread&) = delete;
        Thread(Thread&&) = delete;
        Thread& operator=(Thread&&) = delete;
        Thread& operator=(const Thread&) = delete;

        /**
         * \brief simple worker thread with a unique callback funtion
         * \param function the callback function we will call.
         */
        explicit Thread(const TCallback& function);

        /**
         * \brief create a thread with a worker.
         * \param worker the worker class that will do the actual work.
         */
        explicit Thread(Worker& worker);
        ~Thread();
        void Wait();

        /**
         * \brief wait a little bit for the thread to finish
         * \param timeout the number of ms we want to wait for the thread to complete.
         * \return either timeout of complete if the thread completed.
         */
        WaitResult WaitFor(long long timeout);
      };
    }
  }
}