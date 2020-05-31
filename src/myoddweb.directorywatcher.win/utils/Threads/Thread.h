// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <thread>
#include <functional>
#include <future>


#include "WaitResult.h"
#include "Worker.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace threads
    {
      typedef std::function<void()> TCallback;

      /**
       * \brief worker thread that either uses std::thread or std::future depending on settings.
       *        if uses a worker so we can manage when a game is started/stopped.
       */
      class Thread final
      {
        /**
         * \brief the worker we created when the called passed us only a function.
         */
        Worker* _localWorker;

        /**
         * \brief the worker that the user passed us.
         */
        Worker* _parentWorker;

        /**
         * \brief the future we will be using, (if we are using a future)
         */
        std::future<void>* _future;

        /**
         * \brief the thread we will be using, (if we are using a thread)
         */
        std::thread* _thread;

        /**
         * \brief create the tread/future we will be running the worker with.
         * \param worker the worker we want to run
         */
        void CreateWorkerRunner(Worker* worker);

        /**
         * \brief start running the worker.
         */
        void Start();

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

        /**
         * \brief if the thread is completed or not.
         * \return if completed or not
         */
        [[nodiscard]]
        bool Completed() const;

        /**
         * \brief if the thread is started or not.
         * \return if started or not
         */
        [[nodiscard]]
        bool Started() const;

        /**
         * \brief wait for the thread to complete.
         *        this will never expire.
         */
        void Wait();

        /**
         * \brief wait a little bit for the thread to finish
         * \param timeout the number of ms we want to wait for the thread to complete.
         * \return either timeout of complete if the thread completed.
         */
        WaitResult WaitFor(long long timeout);

      private:
        /**
         * \brief wait a little bit for the thread to finish
         * \param worker the worker we are waiting for.
         * \param timeout the number of ms we want to wait for the thread to complete.
         * \return either timeout of complete if the thread completed.
         */
        static WaitResult WaitFor(Worker* worker, long long timeout);
      };
    }
  }
}