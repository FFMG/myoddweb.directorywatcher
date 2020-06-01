// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;

namespace myoddweb.directorywatcher.interfaces
{
  public interface IWatcher3 : IWatcher2, IDisposable
  {
    /// <summary>
    /// If the watcher is ready or not.
    /// </summary>
    /// <returns></returns>
    bool Ready();
  }
}
