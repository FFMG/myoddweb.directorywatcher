// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief unmanaged implementation of IRequest
     */
    struct Request
    {
      /**
       * \brief the path of the folder we will be monitoring
       */
      std::wstring Path;

      /**
       * \brief if we are recursively monitoring or not.
       */
      bool Recursive;
    };
  }
}
