#pragma once
#include <mutex>
#include "Thread.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace threads
    {
      class WorkerPool : public Worker
      {
        /**
         * \brief the current thread handle, if we have one.
         */
        Thread* _thread;

        /**
         * \brief if we must stop our thread or not
         */
        bool _mustStop;

        /**
         * \brief the locks so we can add/remove runners.
         */
        std::recursive_mutex _lock;

        /**
         * \brief all our runners.
         */
        std::vector<Worker*> _workers;

        /**
         * \brief the workers that have yet to be started
         */
        std::vector<Worker*> _workersWaitingToStart;

        /**
         * \brief all our runners.
         */
        std::vector<Worker*> _runningWorkers;

      public:
        WorkerPool(const WorkerPool&) = delete;
        WorkerPool(WorkerPool&&) = delete;
        const WorkerPool& operator=(const WorkerPool&) = delete;

        WorkerPool();
        virtual ~WorkerPool();

        /**
         * \brief add a worker to our worers pool.
         * \param worker the worker we are trying to add.
         */
        void Add(Worker& worker);

        /**
         * \brief wait a little bit for a worker to finish
         *        if the worker does not exist we just return that it is complete.
         * \param worker the worker we are waiting for
         * \param timeout the number of ms we want to wait for the worker to complete.
         * \return either timeout of complete if the thread completed.
         */
        WaitResult WaitFor(Worker& worker, long long timeout);

        /**
         * \brief wait a little bit for all the workers to finish
         *        if the worker does not exist we just return that it is complete.
         * \param timeout the number of ms we want to wait for the workers to complete.
         * \return either timeout of complete if the threads completed.
         */
        WaitResult WaitFor( long long timeout);

        /**
         * \brief non blocking call to instruct the thread to stop.
         */
        void Stop() override;

      protected:
        bool OnWorkerStart() override;

        /**
         * \brief Give the worker a chance to do something in the loop
         *        Workers can do _all_ the work at once and simply return false
         *        or if they have a tight look they can return true until they need to come out.
         * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
         * \return true if we want to continue or false if we want to end the thread
         */
        bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override;

        void OnWorkerEnd() override;

        /**
         * \brief make a thread safe copy of the running workers.
         */
        std::vector<Worker*> CloneRunningWorkers();

        /**
         * \brief make a thread safe copy of the running workers.
         */
        std::vector<Worker*> CloneWorkersWaitingToStart();

        /**
         * \brief start any workers that are pending.
         */
        void ProcessWorkersWaitingToStart();

        /**
         * \brief process workers that has indicated the need to stop.
         * \param workers the workers we are wanting to stop
         */
        void ProcessWorkersWaitingToStop( std::vector<Worker*>& workers );

        /**
         * \brief stop a single worker and wait for it to complete
         * \param worker the worker we are stopping
         * \param timeout how long we want to wait for.
         * \return either timeout or complete
         */
        WaitResult StopAndRemoveWorker( Worker& worker, long long timeout);

        /**
         * \brief remove a worker from our list of posible waiting workers.
         *        we will obtain the lock to remove this item.
         * \param worker the worker we are wanting to remove
         */
        void RemoveWorkerFromWorkersWaitingStarting( const Worker& worker );

        /**
         * \brief remove a worker from our list of workers.
         *        we will obtain the lock to remove this item.
         * \param worker the worker we are wanting to remove
         */
        void RemoveWorkerFromWorkers(const Worker& worker);

        /**
         * \brief remove a worker from our list of running workers.
         *        we will obtain the lock to remove this item.
         * \param worker the worker we are wanting to remove
         */
        void RemoveWorkerFromRunningWorkers(const Worker& worker);

        /**
         * \brief remove a single worker from a collection of workers
         * \param container the collection of workers.
         * \param item the worker we want t remove
         */
        static void RemoveWorker(std::vector<Worker*>& container, const Worker& item);
      };
    }
  }
}
