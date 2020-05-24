#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace threads
    {
      enum class WaitResult
      {
        // names for timed wait function returns
        complete,
        timeout,
      };
    }
  }
}