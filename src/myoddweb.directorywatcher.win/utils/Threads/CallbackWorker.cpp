// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "CallbackWorker.h"
#include <utility>

#include "../Instrumentor.h"

namespace myoddweb::directorywatcher::threads
{
  CallbackWorker::CallbackWorker(TCallback function) :
    Worker(),
    _function(std::move(function))
  {
  }

  bool CallbackWorker::on_worker_update(float fElapsedTimeMilliseconds)
  {
    MYODDWEB_PROFILE_FUNCTION();

    // call the function
    _function();

    // we are done.
    return false;
  }

  /**
   * \brief called when the worker has completed
   *        this is to allow our workers a chance to dispose of data
   *
   */
  void CallbackWorker::on_worker_end()
  {
    MYODDWEB_PROFILE_FUNCTION();
  }

  /**
   * \brief called when the worker has completed
   *        this is to allow our workers a chance to dispose of data
   *
   */
  void CallbackWorker::on_worker_stop()
  {
    MYODDWEB_PROFILE_FUNCTION();
  }
}
