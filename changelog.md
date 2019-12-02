# Change Log

Notable changes

## 0.1.7 - [Current]

### Added

- Instrumentation, [Thanks to The Cherno / Hazel project)](https://github.com/TheCherno/Hazel/)
  The flag `MYODDWEB_PROFILE` needs to be set to 1.
- Version number fo unmanaged `myoddweb.directorywatcher` helper.

### Changed

- Various performance / memory fixes
  - Some code now running is parallel to speed-up the processing of folders/files that have changed.

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
