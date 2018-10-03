#pragma once
#include <string>

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief implementation of IRequest
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
