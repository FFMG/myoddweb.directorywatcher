using System;
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

      public StatisticsCallback StatisticsCallback;

      [MarshalAs(UnmanagedType.I8)]
      public long EventsCallbackIntervalMs;
      
      [MarshalAs(UnmanagedType.I8)]
      public long StatisticsCallbackIntervalMs;

      public LoggerCallback LoggerCallback;
    }

    // Delegate with function signature for the GetVersion function
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I8)]
    public delegate Int64 Start(ref Request request);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public delegate bool Stop([In, MarshalAs(UnmanagedType.U8)] long id);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public delegate bool Ready();

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void EventsCallback(
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
      [MarshalAs(UnmanagedType.I8)] long id,
      [MarshalAs(UnmanagedType.R8)] double elapsedTime,
      [MarshalAs(UnmanagedType.I8)] long numberOfEvents
    );

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void LoggerCallback(
      [MarshalAs(UnmanagedType.I8)] long id,
      [MarshalAs(UnmanagedType.I4)] int type,
      [MarshalAs(UnmanagedType.LPWStr)] string message
    );
  }
}
