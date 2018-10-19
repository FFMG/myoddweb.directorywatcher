//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using myoddweb.directorywatcher.utils;
using myoddweb.directorywatcher.interfaces;
using EventError = myoddweb.directorywatcher.interfaces.EventError;

namespace myoddweb.directorywatcher
{
  public class Watcher : IWatcher2
  {
    #region Member variables
    /// <summary>
    /// To stop the running task
    /// </summary>
    private CancellationTokenSource _source;

    /// <summary>
    /// The currently running task, (if we have one).
    /// </summary>
    private Task _task;

    /// <summary>
    /// If we started work or not.
    /// </summary>
    private bool _started;

    /// <summary>
    /// The list of ids we already started
    /// </summary>
    private readonly IDictionary<long, IRequest> _processedRequests = new Dictionary<long, IRequest>();

    /// <summary>
    /// The list of requests.
    /// </summary>
    private readonly IList<IRequest> _pendingRequests = new List<IRequest>();
    #endregion

    #region Events
    /// <inheritdoc />
    public event WatcherEvent<IFileSystemEvent> OnTouchedAsync;

    /// <inheritdoc />
    public event WatcherEvent<IFileSystemEvent> OnAddedAsync;

    /// <inheritdoc />
    public event WatcherEvent<IFileSystemEvent> OnRemovedAsync;

    /// <inheritdoc />
    public event WatcherEvent<IRenamedFileSystemEvent> OnRenamedAsync;

    /// <inheritdoc />
    public event WatcherEvent<IEventError> OnErrorAsync;
    #endregion

    #region IWatcher1/IWatcher2
    /// <inheritdoc />
    public long Start(IRequest request)
    {
      // As we have already started the work.
      // so we want to start this one now
      // and add it to our list of started ids.
      var id = WatcherManager.Get.Start(request);
      if (id != -1)
      {
        _processedRequests[id] = request;
      }

      // try and run whatever pending requests we might still have.
      Start();

      // return the id of the newly started request.
      return id;
    }

    /// <inheritdoc />
    public bool Add(IRequest request)
    {
      if (_started)
      {
        // we started already, so add this one now.
        // if it is not negative, then it worked.
        return Start(request) >= 0;
      }

      // we have not started 
      // so just add it to the queue
      _pendingRequests.Add( request );

      // all good.
      return true;
    }

    /// <inheritdoc />
    public bool Stop(long id)
    {
      if (!_processedRequests.ContainsKey(id))
      {
        return false;
      }

      if (!WatcherManager.Get.Stop(id))
      {
        return false;
      }

      // remove it.
      _processedRequests.Remove(id);
      return true;
    }

    /// <inheritdoc />
    public long GetEvents(long id, out IList<IEvent> events )
    {
      return WatcherManager.Get.GetEvents(id, out events );
    }

    /// <inheritdoc />
    public bool Start()
    {
      try
      {
        // do we have anything to do ... or are we even able to work?
        if (!_pendingRequests.Any() || WatcherManager.Get == null)
        {
          return false;
        }

        // assume that some work can be done
        var requestProcessed = false;

        // try and add the requests.
        foreach (var request in _pendingRequests)
        {
          // get the id, we do not want to call
          // our own Add( ... ) function as it checks
          // if we have started work or not.
          var id = WatcherManager.Get.Start(request);
          if (id < 0)
          {
            // negative results mean that it did not work.
            // so we will leave it as pending.
            continue;
          }

          // add this to our processed requests.
          _processedRequests[id] = request;

          // work was done
          requestProcessed = true;
        }

        // was any work done?
        if (!requestProcessed)
        {
          return false;
        }

        // remove what we processed from the pending list.
        foreach (var processedRequest in _processedRequests)
        {
          _pendingRequests.Remove(processedRequest.Value);
        }

        // we started something.
        return true;
      }
      finally
      {
        // regadless what happned we have now started
        // in case the user calls start before adding anything at all.
        StartInternalEvents();
      }
    }

    /// <inheritdoc />
    public bool Stop()
    {
      try
      {
        // do we have any completed requests?
        if (!_processedRequests.Any() || WatcherManager.Get == null)
        {
          return false;
        }

        // we try and remove the requests here.
        // make sure that we clone the list rather than using the actual list
        // because Remove() will actually change _processedRequests itself.
        foreach (var id in _processedRequests.Select(r => r.Key).ToArray())
        {
          Stop(id);
        }
        return true;
      }
      finally
      {
        // we have now stopped.
        StopInternalEvents();
      }
    }

    /// <inheritdoc />
    public long GetEvents(out IList<IEvent> events)
    {
      var allEvents = new List<IEvent>();
      foreach (var id in _processedRequests.Select(r => r.Key).ToArray())
      {
        if (GetEvents(id, out var currentEvents) > 0)
        {
          allEvents.AddRange( currentEvents );
        }
      }
      events = allEvents;
      return allEvents.Count;
    }
    #endregion

    #region Private functions
    private void StopInternalEvents()
    {
      // are we running?
      _source?.Cancel();
      _task?.Wait();

      // flag that this has stoped.
      _started = false;
      _task = null;
    }

    private void StartInternalEvents()
    {
      // stowhatever might still be running.
      StopInternalEvents();

      // flag that this has started.
      _started = true;

      // cancel the task
      _source = new CancellationTokenSource();

      // start the next task
      _task = ProcessEvents( _source.Token );
    }

    /// <summary>
    /// Thread that will process the events and call the watchers.
    /// This is a long running thread that we will keep calling while we have events
    /// or until stop is called.
    /// </summary>
    /// <param name="token"></param>
    /// <returns></returns>
    private async Task<bool> ProcessEvents( CancellationToken token )
    {
      //  how big we want to allow the list of tasks to be.
      // before we 'clean' the completed list.
      const int maxNumTasks = 1028;

      // the list of tasks .
      var tasks = new List<Task>(maxNumTasks);
      try
      {
        // loop around while we process events.
        while (!_source.IsCancellationRequested)
        {
          try
          {
            // get all the events
            if (GetEvents(out var events) <= 0)
            {
              continue;
            }

            // then call the various actions.
            foreach (var e in events)
            {
              // do we have an error.
              if (e.Error != EventError.None)
              {
                if (OnErrorAsync != null)
                {
                  tasks.Add(Task.Run(() =>
                      OnErrorAsync?.Invoke(new utils.EventError(e.Error, e.DateTimeUtc), token)
                    , token));
                }
              }

              switch (e.Action)
              {
                case EventAction.Added:
                  if (OnAddedAsync != null)
                  {
                    tasks.Add(Task.Run(() => 
                      OnAddedAsync?.Invoke(new FileSystemEvent(e), token), token));
                  }
                  break;

                case EventAction.Removed:
                  if (OnRemovedAsync != null)
                  {
                    tasks.Add(Task.Run(() => 
                      OnRemovedAsync?.Invoke(new FileSystemEvent(e), token), token));
                  }
                  break;

                case EventAction.Touched:
                  if (OnTouchedAsync != null)
                  {
                    tasks.Add(Task.Run(() => 
                      OnTouchedAsync?.Invoke(new FileSystemEvent(e), token), token));
                  }
                  break;

                case EventAction.Renamed:
                  if (OnRenamedAsync != null)
                  {
                    tasks.Add(Task.Run(() => 
                      OnRenamedAsync?.Invoke(new RenamedFileSystemEvent(e), token), token));
                  }
                  break;

                default:
                  throw new NotSupportedException( $"Received an unknown Action: {e.Action.ToString("G")}");
              }

              if (tasks.Count > maxNumTasks)
              {
                tasks.RemoveAll(t => t.IsCompleted);
              }
            }
          }
          catch
          {
            try
            {
              // on of the functions threw an error.
              if (OnErrorAsync != null)
              {
                tasks.Add(Task.Run(() => OnErrorAsync(new utils.EventError(EventError.General, DateTime.UtcNow), token), token));
              }
            }
            catch
            {
              // the error .. threw an error...
              // I give up with this error.
            }
          }
          finally
          {
            // wait ... a little.
            await Task.Delay(100, token).ConfigureAwait(false);
          }
        }
      }
      catch (OperationCanceledException e)
      {
        if (e.CancellationToken != _source.Token)
        {
          //  not my token
          throw;
        }
      }
      finally
      {
        // wait for all the tasks to complete.
        tasks.RemoveAll(t => t.IsCompleted);
        Task.WaitAll(tasks.ToArray());
      }
      return true;
    }
    #endregion
  }
}
