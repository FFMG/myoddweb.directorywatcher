# Load testing application

## General

Very simple load application tat allows us to test various aspects of `myoddweb.directorywatcher`

`--help` :  display the help screen

```
Usage: myoddweb.directorywatcher.load.exe
                                           [--iterations, --i=<5>]
                                           [--folders, --f=<5>]
                                           [--change, --c=<10>]
                                           [--unique, --u=<true>]
                                           [--stats, --s=<true>]
                                           [--quiet, --q=<false>]
                                           [--drives, --d=<false>]
                                           [--random, --r=<false>]
                                           [--help, --h]

help, h        :Display the help message
iterations, i  :The number of test itarations we wish to run.
This is the number of time we want to randomly stop/start watching a folder.
folders, f     :The number of folders watched at once.
change, c      :How often we want to change to another folder (in seconds).
unique, u      :If we want to use a unique watcher, shared, or use a watcher per directory.
stats, s       :Display stats every 10 seconds.
quiet, q       :Do not display the folder updates.
drives, d      :Test all the drives only.
random, r      :Randomly create various folders/files and check for update.
```

## Random 

Example: `--iterations 5 --q true --r true`

Run a test for `x` iterations and add/remove files at random intervals, the idea being that changes are still picked up after a while.

We start a watcher that monitors all the folders and all the files, so it is the "maximum" case.