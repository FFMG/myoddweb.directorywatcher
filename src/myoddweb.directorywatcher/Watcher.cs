// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using myoddweb.directorywatcher.utils;
using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.utils.Helper;
using EventError = myoddweb.directorywatcher.interfaces.EventError;

namespace myoddweb.directorywatcher
{
  public class Watcher : IWatcher3
  {
    #region Member variables
    /// <summary>
    /// The default number of ms we want to wait for.
    /// </summary>
    private const int DefaultDelayWait = 500;

    /// <summary>
    /// The number of milliseconds we want to wait between getting events
    /// </summary>
    private int _delayWaitEvents;

    /// <summary>
    /// The number of milliseconds we want to wait between getting stats
    /// </summary>
    private int _delayWaitSatistics;

    /// <summary>
    /// The watch manager that allows us to control who loads what
    /// </summary>
    private readonly WatcherManager _watcherManager;

    /// <summary>
    /// Check if we disposed already.
    /// </summary>
    private bool _disposed;

    /// <summary>
    /// All the running event tasks
    /// </summary>
    private CancellationTokenSource _eventsSource;

    /// <summary>
    /// All the running statistics tasks
    /// </summary>
    private CancellationTokenSource _statisticsSource;

    /// <summary>
    /// The parent watcher source
    /// </summary>
    private readonly CancellationTokenSource _watcherSource = new CancellationTokenSource();
    
    /// <summary>
    /// This is the task that always waits for events to run.
    /// </summary>
    private readonly Task _eventsTask;

    /// <summary>
    /// All the logger taks
    /// </summary>
    private readonly List<Task> _logerTasks = new List<Task>();

    /// <summary>
    /// The long running statistics taks
    /// </summary>
    private readonly Task _statisticsTask;

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

    /// <inheritdoc />
    public event WatcherEvent<IStatistics> OnStatisticsAsync;

    /// <inheritdoc />
    public event WatcherEvent<ILoggerEvent> OnLoggerAsync;
    #endregion

    public Watcher()
    {
      // make sure that the event wait values are set
      RecalculateDelays();

#if DEBUG
      // _watcherManager = new WatcherManagerEmbeddedInterop();
      // _watcherManager = new WatcherManagerInterop();

      // The debug version does not use the embedded version.
      _watcherManager = new WatcherManagerLoadLibrary();
#else
      // The release function must use the embedded version.
      _watcherManager = new WatcherManagerEmbeddedLoadLibrary();
#endif

      // register for logger events right away
      _watcherManager.OnLogger += OnLogger;

      // start the task that will forever be looking for events.
      _eventsTask = ProcessEventsAsync();

      // then run all the statistics task
      _statisticsTask = ProcessStatisticsAsync();

      // we have not disposed
      _disposed = false;

    }

    private void OnLogger(ILoggerEvent logger)
    {
      lock (_logerTasks)
      {
        if (OnLoggerAsync != null)
        {
          var token = _watcherSource.Token;
          var task = OnLoggerAsync != null
            ? Task.Run(() => OnLoggerAsync?.Invoke(logger, token), token)
            : Task.FromResult(false);
          _logerTasks.Add(task);
        }

        _logerTasks.RemoveAll(t => t.IsCompleted);
      }
    }

    /// <summary>
    /// Dispose of resources
    /// </summary>
    ~Watcher()
    {
      Dispose( false );
    }

    #region IDisposable
    /// <summary>
    /// Implement the dispose pattern
    /// </summary>
    public void Dispose() => Dispose(true);

    /// <summary>
    /// Call when we are disposing or resources.
    /// </summary>
    /// <param name="disposing"></param>
    protected virtual void Dispose( bool disposing )
    {
      if (_disposed)
      {
        return;
      }

      try
      {
        // the 'finally' will set the _disposed flag to true.
        if (!disposing)
        {
          return;
        }

        // remove the logger event
        if (_watcherManager != null )
        {
          _watcherManager.OnLogger -= OnLogger;
        }

        // stop collecting events
        Stop();

        // stop everything else
        StopInternalEvents();

        // and dispose...
        _eventsSource?.Dispose();
        _statisticsSource?.Dispose();
        _watcherSource?.Dispose();
        _eventsTask.Dispose();
        _statisticsTask.Dispose();
        _watcherManager?.Dispose();
      }
      finally
      {
        _disposed = true;
      }
    }

    /// <summary>
    /// Check if disposed was called.
    /// </summary>
    internal void CheckDisposed()
    {
      if (_disposed)
      {
        throw new ObjectDisposedException(GetType().FullName);
      }
    }

    private void StopInternalEvents()
    {
      // are we running?
      _eventsSource?.Cancel();
      _statisticsSource?.Cancel();
      _watcherSource?.Cancel();

      // wait for all the tasks to end
      Task.WaitAll(_eventsTask, _statisticsTask);
      Task.WaitAll(_logerTasks.ToArray());

      // flag that this has stoped.
      _started = false;
    }
#endregion

    #region IWatcher1/IWatcher2/IWatcher3
    /// <inheritdoc />
    public long Start(IRequest request)
    {
      // we cannot start what has been disposed.
      CheckDisposed();

      // As we have already started the work.
      // so we want to start this one now
      // and add it to our list of started ids.
      var id = _watcherManager.Start(request);
      if (id != -1)
      {
        _processedRequests[id] = request;
        RecalculateDelays();
      }

      // try and run whatever pending requests we might still have.
      Start();

      // return the id of the newly started request.
      return id;
    }

    /// <inheritdoc />
    public bool Add(IRequest request)
    {
      // we cannot add if we have disposed already
      CheckDisposed();

      if (_started)
      {
        // we started already, so add this one now.
        // if it is not negative, then it worked.
        return Start(request) >= 0;
      }

      // we have not started 
      // so just add it to the queue
      _pendingRequests.Add(request);

      // update our counters.
      RecalculateDelays();

      // all good.
      return true;
    }

    /// <inheritdoc />
    public bool Stop(long id)
    {
      // we cannot stop what has been disposed.
      CheckDisposed();

      if (!_processedRequests.ContainsKey(id))
      {
        return false;
      }

      if (!_watcherManager.Stop(id))
      {
        return false;
      }

      // remove it.
      _processedRequests.Remove(id);

      // update our counters.
      RecalculateDelays();
      return true;
    }

    private void RecalculateDelays()
    {
      RecalculateEventsDelays();
      RecalculateStatisticsDelays();
    }

    private void RecalculateStatisticsDelays()
    {
      if (_processedRequests.Count == 0)
      {
        //  we have nothing to watch ... just wait half a second
        _delayWaitSatistics = DefaultDelayWait;
        return;
      }

      var minWaitStatistics = _processedRequests.Values.Min(r => r.Rates.StatisticsMilliseconds);
      if (minWaitStatistics > int.MaxValue || minWaitStatistics <= 0)
      {
        _delayWaitSatistics = DefaultDelayWait;
      }
      else
      {
        _delayWaitSatistics = (int)minWaitStatistics;
      }
    }

    private void RecalculateEventsDelays()
    {
      if (_processedRequests.Count == 0)
      {
        //  we have nothing to watch ... just wait half a second
        _delayWaitEvents = DefaultDelayWait;
        return;
      }

      var minWaitEvents = _processedRequests.Values.Min(r => r.Rates.EventsMilliseconds);
      if (minWaitEvents > int.MaxValue || minWaitEvents <= 0)
      {
        _delayWaitEvents = DefaultDelayWait;
      }
      else
      {
        _delayWaitEvents = (int)minWaitEvents;
      }
    }

    /// <inheritdoc />
    public bool Ready()
    {
      // we cannot check what has been disposed.
      CheckDisposed();

      try
      {
        // special case, if we have started, but we have no processed requests
        // then we can say that we are ready
        // we know for a fact that the manager is not ready, simply because
        // we could not have started it without any folder.
        // but we are not in error, we are indeed "ready"
        if (_started && !_processedRequests.Any())
        {
          return true;
        }

        // do we have anything to do ... or are we even able to work?
        return _watcherManager?.Ready() ?? false;
      }
      finally
      {
        // regadless what happned we have now started
        // in case the user calls start before adding anything at all.

        // flag that this has started.
        _started = true;

        // the cancellation sources
        _eventsSource = new CancellationTokenSource();
        _statisticsSource = new CancellationTokenSource();
      }
    }

    /// <inheritdoc />
    public bool Start()
    {
      // we cannot start what has been disposed.
      CheckDisposed();

      try
      {
        // do we have anything to do ... or are we even able to work?
        if (!_pendingRequests.Any() || _watcherManager == null)
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
          var id = _watcherManager.Start(request);
          if (id < 0)
          {
            // negative results mean that it did not work.
            // so we will leave it as pending.
            continue;
          }

          // add this to our processed requests.
          _processedRequests[id] = request;
          RecalculateDelays();

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

        RecalculateDelays();

        // we started something.
        return true;
      }
      finally
      {
        // regadless what happned we have now started
        // in case the user calls start before adding anything at all.

        // flag that this has started.
        _started = true;

        // the cancellation task sources
        _eventsSource = new CancellationTokenSource();
        _statisticsSource = new CancellationTokenSource();
      }
    }

    /// <inheritdoc />
    public bool Stop()
    {
      // we cannot start what has been disposed.
      CheckDisposed();

      // do we have any completed requests?
      if (!_processedRequests.Any() || _watcherManager == null)
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

    /// <inheritdoc />
    public bool GetStatistics( long id, out IStatistics statistics)
    {
      // we cannot get the events if we disposed.
      CheckDisposed();

      // we cannot get any more events if we have disposed.
      CheckDisposed();

      return _watcherManager.GetStatistics(id, out statistics);
    }

    /// <inheritdoc />
    public long GetStatistics( out IList<IStatistics> statistics)
    {
      // we cannot get the events if we disposed.
      CheckDisposed();

      var allStatistics = new List<IStatistics>();
      foreach (var id in _processedRequests.Select(r => r.Key).ToArray())
      {
        if (GetStatistics(id, out var currentStatistics))
        {
          allStatistics.Add(currentStatistics);
        }
      }

      statistics = allStatistics;
      return allStatistics.Count;
    }

    /// <inheritdoc />
    public long GetEvents(long id, out IList<IEvent> events)
    {
      // we cannot get any more events if we have disposed.
      CheckDisposed();

      events = _watcherManager.GetEvents(id );
      return events.Count;
    }

    /// <inheritdoc />
    public long GetEvents(out IList<IEvent> events)
    {
      // we cannot get the events if we disposed.
      CheckDisposed();

      var allEvents = new List<IEvent>();
      foreach (var id in _processedRequests.Select(r => r.Key).ToArray())
      {
        if (GetEvents(id, out var currentEvents) > 0)
        {
          allEvents.AddRange(currentEvents);
        }
      }

      events = allEvents;
      return allEvents.Count;
    }
    #endregion

    #region Private functions
    private Task ProcessEvent(IEvent e, CancellationToken token)
    {
      // do we have an error.
      if (e.Error != EventError.None)
      {
        return OnErrorAsync != null ? Task.Run(() => OnErrorAsync?.Invoke(new utils.EventError(e.Error, e.DateTimeUtc), token), token) : Task.FromResult(false);
      }

      switch (e.Action)
      {
        case EventAction.Added:
          return OnAddedAsync != null ? Task.Run(() => OnAddedAsync?.Invoke(new FileSystemEvent(e), token), token) : Task.FromResult(false);

        case EventAction.Removed:
          return OnRemovedAsync != null ? Task.Run(() => OnRemovedAsync?.Invoke(new FileSystemEvent(e), token), token) : Task.FromResult(false);

        case EventAction.Touched:
          return OnTouchedAsync != null ? Task.Run(() => OnTouchedAsync?.Invoke(new FileSystemEvent(e), token), token) : Task.FromResult(false);

        case EventAction.Renamed:
          return OnRenamedAsync != null ? Task.Run(() => OnRenamedAsync?.Invoke(new RenamedFileSystemEvent(e), token), token) : Task.FromResult(false);

        default:
          throw new NotSupportedException($"Received an unknown Action: {e.Action:G}");
      }
    }

    /// <summary>
    /// Create the events for each events.
    /// </summary>
    /// <param name="events"></param>
    /// <param name="maxNumTasks"></param>
    /// <param name="token"></param>
    /// <returns></returns>
    private IEnumerable<Task> ProcessEvents( IEnumerable<IEvent> events, int maxNumTasks, CancellationToken token)
    {
      // the created events.
      var tasks = new List<Task>(maxNumTasks);

      // then call the various actions.
      foreach (var e in events)
      {
        try
        {
          var task = ProcessEvent(e, token);
          if (!task.IsCompleted)
          {
            tasks.Add( task );
          }
        }
        catch
        {
          if (OnErrorAsync != null)
          {
            tasks.Add(Task.Run(() => OnErrorAsync(new utils.EventError(EventError.General, DateTime.UtcNow), token), token));
          }
        }
      }
      return tasks;
    }

    /// <summary>
    /// Process a list of statistics and send them all to the various observers.
    /// </summary>
    /// <param name="statistics"></param>
    /// <param name="maxNumTasks"></param>
    /// <param name="statisticsSourceToken"></param>
    /// <returns></returns>
    private IEnumerable<Task> ProcessStatistics(IEnumerable<IStatistics> statistics, int maxNumTasks, CancellationToken statisticsSourceToken)
    {
      // the created statistics.
      var tasks = new List<Task>(maxNumTasks);

      // then call the various stats.
      foreach (var statistic in statistics)
      {
        try
        {
          var task = OnStatisticsAsync != null
            ? Task.Run(() => OnStatisticsAsync?.Invoke(statistic, statisticsSourceToken), statisticsSourceToken)
            : Task.FromResult(false);
          if (!task.IsCompleted)
          {
            tasks.Add(task);
          }
        }
        catch
        {
          if (OnErrorAsync != null)
          {
            tasks.Add(Task.Run(() => OnErrorAsync(new utils.EventError(EventError.General, DateTime.UtcNow), statisticsSourceToken), statisticsSourceToken));
          }
        }
      }
      return tasks;
    }

    /// <summary>
    /// Long running task to get all the statistics
    /// </summary>
    /// <returns></returns>
    private async Task<bool> ProcessStatisticsAsync()
    {
      //  how big we want to allow the list of tasks to be.
      // before we 'clean' the completed list.
      const int maxNumTasks = 1028;

      // the list of tasks .
      var tasks = new List<Task>(maxNumTasks);
      try
      {
        // loop around while we process events.
        while (!_watcherSource.IsCancellationRequested)
        {
          try
          {
            // no point in doing anything until we are all done.
            if (_statisticsSource?.IsCancellationRequested ?? true)
            {
              continue;
            }

            // get all the events
            if (GetStatistics(out var statistics) <= 0)
            {
              continue;
            }

            // process all the events.
            tasks.AddRange(ProcessStatistics(statistics, maxNumTasks, _statisticsSource.Token));

            // create the tasks that are complete.
            if (tasks.Count > maxNumTasks)
            {
              tasks.RemoveAll(t => t.IsCompleted);
            }
          }
          catch (Exception e)
          {
            if (e is OperationCanceledException oc)
            {
              if (oc.CancellationToken == _statisticsSource?.Token)
              {
                // we just cancelled the events token
                continue;
              }

              if (oc.CancellationToken == _watcherSource.Token)
              {
                // we cancelled the entire watcher.
                break;
              }
            }
          }
          finally
          {
            // wait ... a little.
            await Task.Delay(_delayWaitEvents, _watcherSource.Token).ConfigureAwait(false);
          }
        }
      }
      catch (OperationCanceledException e)
      {
        if (e.CancellationToken != _watcherSource.Token)
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

    /// <summary>
    /// Thread that will process the events and call the watchers.
    /// This is a long running thread that we will keep calling while we have events
    /// or until stop is called.
    /// </summary>
    /// <returns></returns>
    private async Task<bool> ProcessEventsAsync( )
    {
      //  how big we want to allow the list of tasks to be.
      // before we 'clean' the completed list.
      const int maxNumTasks = 1028;

      // the list of tasks .
      var tasks = new List<Task>(maxNumTasks);
      try
      {
        // loop around while we process events.
        while (!_watcherSource.IsCancellationRequested)
        {
          try
          {
            // no point in doing anything until we are all done.
            if ( _eventsSource?.IsCancellationRequested ?? true )
            {
              continue;
            }

            // get all the events
            if (GetEvents(out var events) <= 0)
            {
              continue;
            }

            // process all the events.
            tasks.AddRange( ProcessEvents(events, maxNumTasks, _eventsSource.Token ) );

            // create the tasks that are complete.
            if (tasks.Count > maxNumTasks)
            {
              tasks.RemoveAll(t => t.IsCompleted);
            }
          }
          catch( Exception e )
          {
            if (e is OperationCanceledException oc)
            {
              if (oc.CancellationToken == _eventsSource?.Token)
              {
                // we just cancelled the events token
                continue;
              }

              if (oc.CancellationToken == _watcherSource.Token)
              {
                // we cancelled the entire watcher.
                break;
              }
            }

            try
            {
              // one of the functions threw an error.
              if (OnErrorAsync != null)
              {
                tasks.Add(Task.Run(() => OnErrorAsync(new utils.EventError(EventError.General, DateTime.UtcNow), _eventsSource.Token), _eventsSource?.Token ?? _watcherSource.Token ));
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
            await Task.Delay(_delayWaitSatistics, _watcherSource.Token ).ConfigureAwait(false);
          }
        }
      }
      catch (OperationCanceledException e)
      {
        if (e.CancellationToken != _watcherSource.Token )
        {
          //  not my token
          throw;
        }
      }
      finally
      {
        // wait for all the tasks to complete.
        tasks.RemoveAll(t => t.IsCompleted);
        Task.WaitAll( tasks.ToArray() );
      }
      return true;
    }
    #endregion
  }
}
