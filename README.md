# Myoddweb.Directorywatcher
A fast and reliable File/Directory watcher for c#/c++ to replace the current .NET `FileSystemWatcher` class.

## What it does

- Reliable monitoring of
	-  Renamed files/directories
	-  Deleted files/directories
	-  Created files/directories
- All exceptions are passed back to the caller.
- Non-blocking delegates, if one function takes a long time ... we don't all have to suffer.
- The interface does allow for porting to other platforms.
- No buffer limitations, (well there is, but we play nicely).
- Try and remove duplicates, (where possible).

## What it doesn't do

- Bring me coffee.

## Installing
### Nuget
#### Package manager
`Install-Package MyOddWeb.DirectoryWatcher`

#### CLI
##### .NET
`dotnet add package MyOddWeb.DirectoryWatcher`

#### Packet
`packet add MyOddWeb.DirectoryWatcher`

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

# Example

# Simple Watch

Add all the directories we want to 'observe' 

```csharp
    var watch = new Watcher();
    watch.Add(new Request("c:\\", true));
    watch.Add(new Request("d:\\foo\\bar\\", true));
    watch.Add(new Request("y:\\", true));
```

Then start 

```csharp
    // start watching
    watch.Start();
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
