// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <atomic>
#include <chrono>
#include <exception>
#include <vector>

#include "WaitResult.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace threads
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
        std::atomic<State> _state;
        std::vector<std::exception_ptr> _exceptions;

        /**
         * \brief The timers used to calculate the elapsed time.
         */
        std::chrono::time_point<std::chrono::system_clock> _timePoint1, _timePoint2;

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

        /**
         * \brief Function called to run the thread.
         */
        void Start();

        /**
         * \brief non blocking call to instruct the thread to stop.
         */
        void Stop();

        /**
         * \brief stop the running thread and wait
         */
        virtual WaitResult StopAndWait( long long timeout );

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

      protected:
        friend WorkerPool;

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

        /**
         * \brief called when the worker is ready to start
         *        return false if you do not wish to start the worker.
         */
        virtual bool OnWorkerStart() { return true; };

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
         */
        virtual void OnWorkerEnd() {};

        /**
         * \brief called when stop is called.
         */
        virtual void OnStop() { };

        /**
         * \brief make sure that all operations are safely completed.
         *        does not throw an exception
         */
        virtual void CompleteAllOperations() noexcept;
      };
    }
  }
}
