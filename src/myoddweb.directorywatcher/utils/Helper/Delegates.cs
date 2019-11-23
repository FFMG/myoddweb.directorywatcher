namespace myoddweb.directorywatcher.utils.Helper
{
  internal static class Delegates
  {
    public struct Request
    {
      [MarshalAs(UnmanagedType.LPWStr)]
      public string Path;

      [MarshalAs(UnmanagedType.I1)]
      public bool Recursive;
    }

    // Delegate with function signature for the GetVersion function
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I8)]
    public delegate Int64 Start(ref Request request, Callback callback, [In, MarshalAs(UnmanagedType.U8)] Int64 id);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public delegate bool Stop([In, MarshalAs(UnmanagedType.U8)] Int64 id);

    public delegate int Callback(
      [MarshalAs(UnmanagedType.I8)] long id,
      [MarshalAs(UnmanagedType.Bool)] bool isFile,
      [MarshalAs(UnmanagedType.LPWStr)] string name,
      [MarshalAs(UnmanagedType.LPWStr)] string oldName,
      [MarshalAs(UnmanagedType.I4)] int action,
      [MarshalAs(UnmanagedType.I4)] int error,
      [MarshalAs(UnmanagedType.I8)] long dateTimeUtc);
  }
}
