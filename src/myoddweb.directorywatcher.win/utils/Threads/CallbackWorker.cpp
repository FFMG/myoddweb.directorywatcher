// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "CallbackWorker.h"
#include <utility>

namespace myoddweb::directorywatcher::threads
{
  CallbackWorker::CallbackWorker(TCallback function) :
    _function(std::move(function))
  {
  }

  bool CallbackWorker::OnWorkerUpdate(float fElapsedTimeMilliseconds)
  {
    // call the function
    _function();

    // we are done.
    return false;
  }
}