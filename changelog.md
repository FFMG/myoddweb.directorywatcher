# Change Log

Notable changes

## 0.1.8 - xx-yy-2020

### Added

- Added worker pool to [limit the number thread](https://github.com/FFMG/myoddweb.directorywatcher/issues/8)
- Added `Ready()` to interface so we can ask if/when the system is ready to monitor files/folders.
- Added basic statistics, (see `IStatistics`), to the directory watcher added event `OnStatisticsAsync`
  - Number of events
  - Number of milliseconds since last statistics
- Added `IRates` interface to allow to control how often events are published.
- Added simple logger so messages can be sent to the watcher event `OnLoggerAsync`

### Changed

- Changed the default number of threads from unlimited to a max of 200, (in reality only about 50)
- You can now set the events rate, the default was every 50ms

### Fixed

- When monitoring folders that were deleted we would not remove them and free memory

### Removed

## 0.1.7 - 03-01-2020

### Added

- Instrumentation, [Thanks to The Cherno / Hazel project)](https://github.com/TheCherno/Hazel/)
  The flag `MYODDWEB_PROFILE` needs to be set to 1.
- Version number for unmanaged `myoddweb.directorywatcher` helper.
- A whole lot of unit tests.

### Changed

- Helper is no longer backward compatible! If you are using another app to call the helper you will need to use my sample(s) to reconnect.
- Various code optimisations.
- Various performance / memory fixes
  - Some code now running is parallel to speed-up the processing of folders/files that have changed.
- Minor other improvements

### Fixed

- If a callback throws an error ... or misbehaves, it no longer kills the entire process.
- There was a bug in 0.1.6 where multiple (sub)folders watcher are created.

### Removed

## 0.1.6 - 24-11-2019

### Added

### Changed

- Trying to fix the loading of embedded files.

### Fixed

- Replaced /MD > /MT to insure smoother loading of embedded files.

### Removed

- The watcher manager used to write to console on error ... but we might not have a console.
- Removed the no longer needed 'interop' project.

## 0.1.5 - 15-11-2019

### Added

- Support for:
  - .NET Core 3.0
  - .NET Standard 2.0
  - .NET 4.6.2

### Changed

### Fixed

### Removed

## 0.1.4 - 13-11-2019

### Added

### Changed

- There was a possibility for `Stop()/Start()` to hang if called at the wrong time.

### Fixed

### Removed

## 0.1.3 - 19-10-2018

### Added

### Changed

### Fixed

- Small changes/fixes picked up during testing and development of [Myoddweb Desktopsearch](https://github.com/FFMG/myoddweb.desktopsearch).

### Removed

## 0.0.4 - 09-10-2018

### Added

- Added a couple of interfaces
  - `IFileSystemEvent.Is( ... )`
  - `IFileSystemEvent.FullName`
  - `IFileSystemEvent.Name`

### Changed

### Fixed

- Better managing of Directories/Files being updated
- Added Google test
- General clean up

### Removed

## 0.0.1.1 - 07-10-2018

### Added

- Initial release

### Changed

### Fixed

### Removed
