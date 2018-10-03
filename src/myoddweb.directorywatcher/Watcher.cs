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
using System.Collections.Generic;
using System.Linq;
using myoddweb.directorywatcher.utils;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher
{
  public class Watcher : IWatcher2
  {
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
        _started = true;
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
        _started = false;
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
  }
}
