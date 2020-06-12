# Myoddweb.Directorywatcher [![Release](https://img.shields.io/badge/release-v0.1.8-brightgreen.png?style=flat)](https://github.com/FFMG/myoddweb.directorywatcher/)

A fast and reliable File/Directory watcher for c#/c++ to replace the current .NET `FileSystemWatcher` class.

## What it does

- Reliable monitoring of
  - Renamed files/directories
  - Deleted files/directories
  - Created files/directories
- All exceptions are passed back to the caller.
- Non-blocking delegates, if one function takes a long time ... we don't all have to suffer.
- The interface does allow for porting to other platforms.
- No buffer limitations, (well there is, but we play nicely).
- Try and remove duplicates, (where possible).
- Deleted, (then re-created), are re-monitored.
- Watcher statistics

## What it doesn't do

- Bring me coffee.

## Installing

### Nuget

[![NuGet Status](https://img.shields.io/nuget/v/MyOddWeb.DirectoryWatcher.svg)](https://www.nuget.org/packages/MyOddWeb.DirectoryWatcher/)
[![NuGet Count](https://img.shields.io/nuget/dt/MyOddWeb.DirectoryWatcher.svg)](https://www.nuget.org/packages/MyOddWeb.DirectoryWatcher/)

#### Package manager

`Install-Package MyOddWeb.DirectoryWatcher`

#### CLI

##### .NET

`dotnet add package MyOddWeb.DirectoryWatcher`

#### Packet

`paket add MyOddWeb.DirectoryWatcher`

### Use case

My needs were to, reliably, monitor entire volumes for created/deleted/renamed files.
I do really care for pattern matching.

## The issue(s) with FileSystemWatcher

The current version of [File Watcher](https://docs.microsoft.com/en-us/dotnet/api/system.io.filesystemwatcher?view=netframework-4.7.2) is great, but it does have a couple of issues.

- There is a buffer limitation, (in the API itself), and a badly written application can 'block' or 'miss' certain notification.
- Duplicates are often sent, (when a file is updated 3 times between calls, we only need to know about the once).
- Certain Exceptions cause the entire app to close.
- UNC/Unix files are not supported, (in fact it causes `FileSystemWatcher` to take your system down).
- Does not handle large volumes nicely.  

## Examples

### Simple Watch

Add all the directories we want to 'observe'

```csharp
    using( var watch = new Watcher() )
    {
      watch.Add(new Request("c:\\", true));
      watch.Add(new Request("d:\\foo\\bar\\", true));
      watch.Add(new Request("y:\\", true));

      // do something amazing with the data
      watch.OnAddedAsync += async (f, t) =>
      {
        // ..
      };

      // start watching
      watch.Start();

      // add some more
       watch.Add(new Request("z:\\", false));

      // optional stop in this case
      watch.Stop();
    }
```

You can start watching at any point

```csharp
    // create Watcher
    var watcher = new Watcher();

    // Add a request.
    watch.Add(new Request("y:\\", true));

    // start watching
    watch.Start();

    // add some more
    watch.Add(new Request("z:\\", false));
```

Get notifications in case a file is created.

```csharp
    watch.OnAddedAsync += async (f, t) =>
    {
      Console.ForegroundColor = ConsoleColor.Green;
      Console.WriteLine(
        $"[{f.DateTimeUtc.Hour}:{f.DateTimeUtc.Minute}:{f.DateTimeUtc.Second}]:{f.FileSystemInfo}");
      Console.ForegroundColor = foreground;
    };
```

we get given the file that was added as well as a cancellation token

And when we are done stop it ...

```csharp
    watch.Stop();
```

Or Dispose of it

```csharp
    watch.Dispose();
```

### Your own 'Watcher' interface

You can create your own watcher interface

```csharp
public class Watcher : IWatcher3
{
  // Implement IWatcher3
}
```

### Watched Events

When a file event is raised we send a `IFileSystemEvent` event.

```csharp
    /// <summary>
    /// The file system event.
    /// </summary>
    FileSystemInfo FileSystemInfo { get; }

    /// <summary>
    ///  Gets the full path of the directory or file.
    /// </summary>
    /// <returns>A string containing the full path.</returns>
    string FullName { get; }

    /// <summary>
    ///     For files, gets the name of the file. For directories, gets the name of the last
    ///     directory in the hierarchy if a hierarchy exists. Otherwise, the Name property
    ///     gets the name of the directory.
    /// </summary>
    /// <returns>A string that is the name of the parent directory, the name of the last directory
    ///     in the hierarchy, or the name of a file, including the file name extension.
    /// </returns>
    string Name { get; }

    /// <summary>
    /// The Action
    ///  Added
    ///  Removed
    ///  Touched
    ///  Renamed
    /// </summary>
    EventAction Action { get; }

    /// <summary>
    /// An error code related to the event, (if any)
    /// </summary>
    EventError Error { get; }

    /// <summary>
    /// The UTC date time of the event.
    /// </summary>
    DateTime DateTimeUtc { get; }

    /// <summary>
    /// Boolean if the update is a file or a directory.
    /// </summary>
    bool IsFile { get; }

    /// <summary>
    /// Return if the event is a certain action
    /// (same as Action == action)
    /// </summary>
    /// <param name="action"></param>
    /// <returns></returns>
    bool Is(EventAction action );
```

### Statistics

You can get statistics at various intervals for the events being watched.

All you need to do is add `Rates` to your watchers

```csharp
    using( var watch = new Watcher() )
    {
      // watch the folder with stats every 10000 ms
      // a value of 0, (default), turns it off.
      watch.Add(new Request("c:\\", true, new Rates(10000, 50 )));

      // do something amazing with the statistics
      // the values is `IStatistics` with a cancellation token
      watch.OnStatisticsAsync += async (s, t) =>
      {
        // ..
      };

      // start watching
      watch.Start();

      // ... do some clever stuff.

      // optional stop in this case
      watch.Stop();
    }
```

### Logger

You can watch for certain events

```csharp
    using( var watch = new Watcher() )
    {
      // watch the folder with stats every 10000 ms
      // a value of 0, (default), turns it off.
      watch.Add(new Request("c:\\", true ));

      // do something amazing with the message
      // the values is `ILoggerEvent` with a cancellation token
      watch.OnLoggerAsync += async (e, t) =>
      {
        // ..
      };

      // start watching
      watch.Start();

      // ... do some clever stuff.

      // optional stop in this case
      watch.Stop();
    }
```
