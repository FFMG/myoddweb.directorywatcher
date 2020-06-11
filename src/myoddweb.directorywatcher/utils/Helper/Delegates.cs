﻿using System;
using System.Runtime.InteropServices;

namespace myoddweb.directorywatcher.utils.Helper
{
  internal class Delegates
  {
    public struct Request
    {
      [MarshalAs(UnmanagedType.LPWStr)]
      public string Path;

      [MarshalAs(UnmanagedType.I1)]
      public bool Recursive;

      public EventsCallback EventsCallback;

      [MarshalAs(UnmanagedType.I8)]
      public Int64 CallbackIntervalMs;
    }

    // Delegate with function signature for the GetVersion function
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I8)]
    public delegate Int64 Start(ref Request request);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public delegate bool Stop([In, MarshalAs(UnmanagedType.U8)] Int64 id);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public delegate bool Ready();

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate int EventsCallback(
      [MarshalAs(UnmanagedType.I8)] long id,
      [MarshalAs(UnmanagedType.Bool)] bool isFile,
      [MarshalAs(UnmanagedType.LPWStr)] string name,
      [MarshalAs(UnmanagedType.LPWStr)] string oldName,
      [MarshalAs(UnmanagedType.I4)] int action,
      [MarshalAs(UnmanagedType.I4)] int error,
      [MarshalAs(UnmanagedType.I8)] long dateTimeUtc
    );

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void StatisticsCallback(
      [MarshalAs(UnmanagedType.I8)] long elapsedTime,
      [MarshalAs(UnmanagedType.I8)] long numberOfEvents,
      [MarshalAs(UnmanagedType.I8)] long actualNumberOfMonitors
    );
  }
}
