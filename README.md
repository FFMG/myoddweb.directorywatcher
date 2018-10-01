# Myoddweb.Directorywatcher
A fast and reliable File/Directory watcher for c#/c++ to replace the current .NET `FileSystemWatcher` class.

## What it does

- Reliable monitoring of
	-  Renamed files/directories
	-  Deleted files/directories
	-  Created files/directories
- All exceptions are passed back to the caller.
- The interface does allow for porting to other platforms.
- No buffer limitations.
- Try and remove duplicates, (where possible)

### Use case

My needs were to, reliably, monitor entire volumes for created/deleted/renamed files.
I do really care for pattern matching.

## What it doesn't do

- Bring me coffee.

## The issue with FileSystemWatcher

The current version of [File Watcher](https://docs.microsoft.com/en-us/dotnet/api/system.io.filesystemwatcher?view=netframework-4.7.2) is great, but it does have a couple of issues.

- There is some sort of buffer limitation
- Duplicates are often sent back/forth
- Certain Exceptions cause the entire app to close.
- UNC/Unix files are not supported, (in fact it causes `FileSystemWatcher` to take your system down).  

