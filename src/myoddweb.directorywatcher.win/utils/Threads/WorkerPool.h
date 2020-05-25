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
         * \brief the lock for the running workers.
         */
        std::recursive_mutex _lockRunningWorkers;

        /**
         * \brief the workers that are waiting to start;
         */
        std::recursive_mutex _lockWorkersWaitingToStart;

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

        /**
         * \brief stop the running thread and wait
         */
        WaitResult StopAndWait(long long timeout) override;

      protected:
        /**
         * \brief called when the worker thread is about to start
         */
        bool OnWorkerStart() override;

        /**
         * \brief Give the worker a chance to do something in the loop
         *        Workers can do _all_ the work at once and simply return false
         *        or if they have a tight look they can return true until they need to come out.
         * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
         * \return true if we want to continue or false if we want to end the thread
         */
        bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override;

        /**
         * \brief Called when the thread pool has been completed, all the workers should have completed here.
         *        We are done with all of them now.
         */
        void OnWorkerEnd() override;

      private:
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
         * \brief process a worker that has been completed.
         * \param worker the workers we are wanting to stop
         */
        void ProcessWorkerWaitingToStop( Worker& worker);

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
        void RemoveWorkerFromWorkersWaitingToStart( const Worker& worker );

        /**
         * \brief remove a workers from our list of posible waiting workers.
         *        we will obtain the lock to remove those items.
         * \param workers the worker we are wanting to remove
         */
        void RemoveWorkerFromWorkersWaitingToStart(const std::vector<Worker*>& workers);

        /**
         * \brief remove a worker from our list of running workers.
         *        we will obtain the lock to remove this item.
         * \param worker the worker we are wanting to remove
         */
        void RemoveWorkerFromRunningWorkers(const Worker& worker);

        /**
         * \brief remove workers from our list of running workers.
         *        we will obtain the lock to remove this items.
         * \param workers the workers we are wanting to remove
         */
        void RemoveWorkerFromRunningWorkers(const std::vector<Worker*>& workers);

        /**
         * \brief remove a single worker from a collection of workers
         * \param container the collection of workers.
         * \param item the worker we want t remove
         */
        static void RemoveWorker(std::vector<Worker*>& container, const Worker& item);

        /**
         * \brief add a single worker to a list of workers that are waiting to start.
         * \param worker the worker we want to add.
         */
        void AddToWorkersWaitingToStart(Worker& worker);

        /**
         * \brief add this worker to our list of running workers
         * \param worker the worker we are adding
         */
        void AddToRunningWorkers(Worker& worker);

        /**
         * \brief had a worker to the container
         * \param container the container we are adding to
         * \param item the worker we want to add.
         */
        static void AddWorker(std::vector<Worker*>& container, Worker& item);
      };
    }
  }
}
