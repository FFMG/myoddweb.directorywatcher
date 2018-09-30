#include "stdafx.h"
#include "CoreWatcher.h"

#include <string>
#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>
#include "Errors.h"
#include "Functions.h"

using namespace msclr::interop;
using namespace System;
using namespace System::Diagnostics;

using namespace System::IO;

CoreWatcher::CoreWatcher() :
  _hDll( nullptr )
{
  if( !CreateInstance() )
  {
    throw new std::exception("Unable to Create core instance.");
  }

  if( !CreateUnmanagedFunctions() )
  {
    throw new std::exception("Unable to Create core unmanaged functions.");
  }
}

CoreWatcher::~CoreWatcher()
{
  Release();
}

void CoreWatcher::Release()
{
  // clean up if need be.
  if (nullptr == _hDll)
  {
    //  we don't try and release the instance if it was never needed.
    return;
  }

  // we can now free everything.
  FreeLibrary(_hDll);
  _hDll = nullptr;
}

bool CoreWatcher::CreateInstance()
{
  //  do we have it already?
  if (nullptr != _hDll)
  {
    // free the library and clear the value.
    FreeLibrary(_hDll);
    _hDll = nullptr;
  }

  // load the library
  auto directoryName = Directory::GetCurrentDirectory();
  auto corePath = Path::Combine(directoryName, "Win32\\myoddweb.directorywatcher.win.dll");
  if (Environment::Is64BitProcess)
  {
    corePath = Path::Combine(directoryName, "x64\\myoddweb.directorywatcher.win.dll");
  }

  msclr::interop::marshal_context context;
  auto std_corePath = context.marshal_as<std::string>(corePath);
  _hDll = LoadLibraryA(std_corePath.c_str());
  if (nullptr == _hDll)
  {
    //  could not load the dll.
    return false;
  }
  return true;
}

/**
 * Load all the unmaneged functions and make sure that they are placed ub the unordered map
 * @return boolean if anything went wrong.
 */
bool CoreWatcher::CreateUnmanagedFunctions()
{
  if (nullptr == _hDll)
  {
    return false;
  }

  // load all the functions.
  for (int i = FunctionTypes::FunctionFirst; i < FunctionTypes::FunctionLast; ++i)
  {
    // try and load that function
    if (!CreateUnmanagedFunction(_hDll, static_cast<FunctionTypes>(i)))
    {
      // something clearly did not work for one of the functions.
      // maybe we did not map a new value?
      return false;
    }
  }

  // all good.
  return true;
}

bool CoreWatcher::CreateUnmanagedFunction(HINSTANCE hInstance, FunctionTypes procType)
{
  FARPROC procAddress = nullptr;
  switch( procType )
  {
  case FunctionTypes::FunctionMonitor:
    procAddress = GetProcAddress(hInstance, "monitor");
    break;

  default:
    auto s = marshal_as<std::string>(
      String::Format("Could not locate the name of the given unmanaged function id. {0}", (int)procType)
      );
    throw new std::invalid_argument(s);
  }

  // save it, for next time.
  if (procAddress == nullptr)
  {
    return false;
  }

  // add it to our list.
  _farProcs[procType] = procAddress;
  return true;
}

/**
 * Get an unmanaged function
 * We will throw if the function does not exist. But this should never happen
 * As we check for all the functions at Load time.
 * @param ProcType procType the function id we are looking for.
 * @return const FARPROC the proc
 */
const FARPROC CoreWatcher::GetUnmanagedFunction(FunctionTypes procType) const
{
  //  check if we have already loaded this function.
  auto it = _farProcs.find(procType);
  if (it == _farProcs.end())
  {
    throw new std::exception("Could not locate proc.");
  }
  return it->second;
}

/*
 * The path we wish to monitor for changes
 * <param name="path">The path we want to monitor.</param>
 * <returns>Unique Id used to release/stop monitoring</returns>
 */ 
__int64 CoreWatcher::Monitor(String^ path, bool recursive)
{
  try
  {
    /// does it exist?
    if (!Directory::Exists(path))
    {
      return Errors::ErrorFolderNotFound;
    }

    // get the function
    auto funci = (f_Monitor)GetUnmanagedFunction(FunctionTypes::FunctionMonitor);

    // otherwise just monitor
    std::wstring wPath = marshal_as<std::wstring>(path);
    return funci(wPath.c_str(), recursive );
  }
  catch( ... )
  {
    return Errors::ErrorUnknown;
  }
  return 0;
}
