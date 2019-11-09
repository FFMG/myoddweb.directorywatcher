// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
namespace myoddweb
{
  namespace directorywatcher
  {
    enum class EventError
    {
      /**
       * \brief No error
       */
      None = 0,

      /**
       * \brief General error
       */
      General = 1,

      /**
       * \brief General memory error, (out of and so on).
       */
      Memory = 2,

      /**
       * \brief there was an overflow.
       */
      Overflow = 3,

      /**
       * \brief the monitoring was stopped somehow.
       */
      Aborted = 4,

      /**
       * \brief Unable to even start the monitoring
       *        Is the path valid? Is the filename valid?
       */
      CannotStart = 5,

      /**
       * \brief Cannot access the file/folder
       */
      Access = 6,

      /**
       * \brief We did not have any file data
       */
      NoFileData = 7,

      /**
       * \brief We could not stop the monitor?
       */
      CannotStop = 8,
    };
  }
}