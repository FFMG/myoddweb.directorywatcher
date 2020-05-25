#pragma once
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
        bool _started;
        bool _completed;
        std::vector<std::exception> _exceptions;

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
        virtual ~Worker() = default;

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
         * \brief Function called to run the thread.
         */
        void Launch();

        /**
         * \brief non blocking call to instruct the thread to stop.
         */
        virtual void Stop() = 0;

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

      protected:
        friend WorkerPool;

        virtual bool OnWorkerStart() { return true; };

        /**
         * \brief Give the worker a chance to do something in the loop
         *        Workers can do _all_ the work at once and simply return false
         *        or if they have a tight look they can return true until they need to come out.
         * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
         * \return true if we want to continue or false if we want to end the thread
         */
        virtual bool OnWorkerUpdate(float fElapsedTimeMilliseconds) = 0;
        virtual void OnWorkerEnd() {};
      };
    }
  }
}
