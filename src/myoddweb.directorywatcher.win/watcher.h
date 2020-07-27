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
    struct sRequest
    {
      /**
           * \brief the path of the folder we will be monitoring
           */
      wchar_t* Path;

      /**
       * \brief if we are recursively monitoring or not.
       */
      bool Recursive;

      /**
       * \brief the callback even we want to call from time to time.
       */
      EventCallback EventsCallback;

      /**
       * \brief the callback even we want to call from time to time.
       */
      StatisticsCallback StatisticsCallback;

      /**
       * How often we wish to callback events
       */
      long long EventsCallbackRateMs;

      /**
       * How often we wish to callback stats
       */
      long long StatisticsCallbackRateMs;

      /**
       * \brief the logger callback
       */
      LoggerCallback LoggerCallback;
    };
  }

  /**
   */
  extern "C" { __declspec(dllexport) bool SetConfig(const sRequest& request); }

  /**
   * \brief Start watching a folder
   * \param request The request containing info about the item we are watching.
   * \return The id of the created request or -ve otherwise
   */
  extern "C" { __declspec(dllexport) long long Start(const sRequest& request); }

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