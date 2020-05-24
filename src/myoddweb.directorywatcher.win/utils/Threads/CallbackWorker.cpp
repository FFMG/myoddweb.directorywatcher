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