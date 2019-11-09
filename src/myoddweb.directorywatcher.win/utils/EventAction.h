// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    enum class EventAction
    {
      /// <summary>
      /// We have an unknown file event.
      /// </summary>
      Unknown = 1000,

      /// <summary>
      /// A file folder was added.
      /// </summary>
      Added = 1001,

      /// <summary>
      /// A file folder was removed
      /// </summary>
      Removed = 1002,

      /// <summary>
      /// Small changed, timestamp, attribute etc...
      /// </summary>
      Touched = 1003,

      /// <summary>
      /// A file folder was renamed.
      /// </summary>
      Renamed = 1004
    };
  }
}
