// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "../../monitors/win/Data.h"

namespace myoddweb:: directorywatcher:: threads
{
  /**
   * \brief class to run a function via a worker
   */
  class CallbackWorker final : public Worker
  {
    TCallback _function;
  public:
    explicit CallbackWorker(TCallback function);
    virtual ~CallbackWorker() = default;

    CallbackWorker(const CallbackWorker&) = delete;
    CallbackWorker(CallbackWorker&&) = delete;
    CallbackWorker&& operator=(CallbackWorker&&) = delete;
    const CallbackWorker& operator=(const CallbackWorker&) = delete;

  protected:
    /**
     * \brief called when the worker is ready to start
     *        return false if you do not wish to start the worker.
     */
    bool OnWorkerStart() override { return true; }

    /**
     * \brief Give the worker a chance to do something in the loop
     *        Workers can do _all_ the work at once and simply return false
     *        or if they have a tight look they can return true until they need to come out.
     * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
     * \return true if we want to continue or false if we want to end the thread
     */
    bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override;

    /**
     * \brief called when the worker has completed
     *        this is to allow our workers a chance to dispose of data
     *
     */
    void OnWorkerEnd() override {}

    /**
     * \brief called when stop is called.
     *        this is to allow our derived workers to stop
     */
    void OnWorkerStop() override {}
  };
}