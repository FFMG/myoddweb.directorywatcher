// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "monitors/Callbacks.h"

namespace myoddweb:: directorywatcher
{
  /**
   *   NB: THE ORDER OF THE VARIABLES IS IMPORTANT!
   *       As set in the Delegates.cs file
   *   public struct Request
   *   {
   *     ...
   *   }
   */
  extern "C" {
    struct raw_request
    {
      /**
           * \brief the path of the folder we will be monitoring
           */
      wchar_t* path;

      /**
       * \brief if we are recursively monitoring or not.
       */
      bool recursive;

      /**
       * \brief the callback even we want to call from time to time.
       */
      EventCallback eventsCallback;

      /**
       * \brief the callback even we want to call from time to time.
       */
      StatisticsCallback statisticsCallback;

      /**
       * How often we wish to callback events
       */
      long long eventsCallbackRateMs;

      /**
       * How often we wish to callback stats
       */
      long long statisticsCallbackRateMs;

      /**
       * \brief the logger callback
       */
      LoggerCallback loggerCallback;
    };
  }

  /**
   * \brief this extern "C" boundary, (Start/Stop/Ready/SetConfig), is the DLL's ABI:
   *        the managed side looks these symbols up by their exact, literal name, so
   *        they intentionally keep their original PascalCase names rather than the
   *        snake_case used for the rest of this codebase.
   */
  extern "C" { __declspec(dllexport) bool SetConfig(const raw_request& request); }

  /**
   * \brief Start watching a folder
   * \param request The request containing info about the item we are watching.
   * \return The id of the created request or -ve otherwise
   */
  extern "C" { __declspec(dllexport) long long Start(const raw_request& request); }

  /**
   * \brief stop watching
   * \param id the id we would like to remove.
   * \return success or not
   */
  extern "C" { __declspec(dllexport) bool Stop(long long id); }

  /**
   * \brief If the monitor manager is ready or not.
   * \return if it is ready or not.
   */
  extern "C" { __declspec(dllexport) bool Ready(); }
}
