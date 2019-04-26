//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
#include <string>
#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>
#include "Watcher1.h"
#include "Errors.h"
#include "Functions.h"
#include "FunctionTypes.h"

using namespace msclr::interop;
using namespace System;
using namespace System::Diagnostics;
using namespace System::Reflection;
using namespace System::IO;

Watcher1::Watcher1() :
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

Watcher1::~Watcher1()
{
  Release();
}

void Watcher1::Release()
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

/**
 * \brief try and load from subfolders of the exe.
 * Non-Embeded dlls will use the one in the platform specific sub-folders.
 * \param core the name of the dll we want to load.
 */
bool Watcher1::CreateInstanceFromPlatformFolder(String^ core)
{
  // load the library from subdirectories
  auto directoryName = Directory::GetCurrentDirectory();
  auto corePath = Path::Combine(directoryName, "Win32\\");
  if (Environment::Is64BitProcess)
  {
    corePath = Path::Combine(directoryName, "x64\\");
  }

  // get the directory and try and load it,
  String^ dll = Path::Combine(Path::GetDirectoryName(corePath), core);

  msclr::interop::marshal_context context;
  const auto stdDll = context.marshal_as<std::wstring>(dll);
  _hDll = LoadLibraryW(stdDll.c_str());
  return (nullptr != _hDll);
}

/**
 * \brief try and load from the same folder as where this dll is located.
 * Embeded dlls are moved to a temp folder
 * So we have to make sure it is loaded from that same folder.
 * \param core the name of the dll we want to load.
 */
bool Watcher1::CreateInstanceFromLocalFolder(String^ core)
{
  // get our 'executing' assembly, (in other words, this dll)
  System::Reflection::Assembly^ callerAssembly = Assembly::GetExecutingAssembly();
  if( callerAssembly == nullptr )
  {
    return false;
  }

  // get the path
  String^ strCallerPath = callerAssembly->Location;
  if (strCallerPath == nullptr)
  {
    return false;
  }

  // get the directory and try and load it,
  String^ dll = Path::Combine( Path::GetDirectoryName(strCallerPath), core );

  msclr::interop::marshal_context context;
  const auto stdDll = context.marshal_as<std::wstring>(dll);
  _hDll = LoadLibraryW(stdDll.c_str());
  return (nullptr != _hDll);
}

bool Watcher1::CreateInstance()
{
  //  do we have it already?
  if (nullptr != _hDll)
  {
    // free the library and clear the value.
    FreeLibrary(_hDll);
    _hDll = nullptr;
  }

  // the core name
  String^ core = Environment::Is64BitProcess ? "myoddweb.directorywatcher.win.x64.dll" : "myoddweb.directorywatcher.win.x86.dll";

  // try and load from the same folder as us
  if( CreateInstanceFromLocalFolder( core ) )
  {
    return true;
  }

  // try and load from the subfolders.
  if (CreateInstanceFromPlatformFolder(core))
  {
    return true;
  }

  return false;
}

/**
 * Load all the unmaneged functions and make sure that they are placed ub the unordered map
 * @return boolean if anything went wrong.
 */
bool Watcher1::CreateUnmanagedFunctions()
{
  if (nullptr == _hDll)
  {
    return false;
  }

  // load all the functions.
  for (auto i = (int)FunctionTypes::FunctionFirst; i < (int)FunctionTypes::FunctionLast; ++i)
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

bool Watcher1::CreateUnmanagedFunction(HINSTANCE hInstance, FunctionTypes procType)
{
  FARPROC procAddress = nullptr;
  switch( procType )
  {
  case FunctionTypes::FunctionStart:
    procAddress = GetProcAddress(hInstance, "Start");
    break;

  case FunctionTypes::FunctionStop:
    procAddress = GetProcAddress(hInstance, "Stop");
    break;

  case FunctionTypes::FunctionGetEvents:
    procAddress = GetProcAddress(hInstance, "GetEvents");
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
const FARPROC Watcher1::GetUnmanagedFunction(FunctionTypes procType) const
{
  //  check if we have already loaded this function.
  auto it = _farProcs.find(procType);
  if (it == _farProcs.end())
  {
    throw new std::exception("Could not locate proc.");
  }
  return it->second;
}

/**
 * \brief Add a path to the list of paths
 * \param The path we want to monitor.
 * \returns
 */ 
long long Watcher1::Start(myoddweb::directorywatcher::interfaces::IRequest^ request )
{
  try
  {
    /// does it exist?
    if (!Directory::Exists( request->Path ))
    {
      return Errors::ErrorFolderNotFound;
    }

    // get the function
    auto funci = (f_Start)GetUnmanagedFunction(FunctionTypes::FunctionStart);

    msclr::interop::marshal_context context;
    UmRequest umRequest;
    umRequest.Path = context.marshal_as<std::wstring>(request->Path);
    umRequest.Recursive = request->Recursive;

    // otherwise just monitor
    return funci(umRequest);
  }
  catch( ... )
  {
    return Errors::ErrorUnknown;
  }
  return 0;
}

/**
 * \brief Remove a path to the list of paths
 * \param The id we want to remove.
 * \returns
 */
bool Watcher1::Stop( __int64 id )
{
  try
  {
    // get the function
    auto funci = (f_Stop)GetUnmanagedFunction(FunctionTypes::FunctionStop);

    // otherwise just monitor
    return funci(id );
  }
  catch (...)
  {
    return false;
  }
  return false;
}

/**
 * \brief Get the latest available events.
 * \param id the id we are looking for.
 * \param cb
 * \return the number of items returned.
 */
long long Watcher1::GetEvents(long long id, IList<myoddweb::directorywatcher::interfaces::IEvent^> ^% events)
{
  // get the function
  auto funci = (f_GetEvents)GetUnmanagedFunction(FunctionTypes::FunctionGetEvents);

  std::vector<UmEvent> umEvents;
  auto result = funci( id, umEvents);
  if( result <= 0 )
  {
    return result;
  }

  /**
   * \brief the date on the 01/01/1970 so we can create DateTime from milliseconds.
   */
  const System::DateTime dateTime1970 = System::DateTime(1970, 1, 1);

  // we can recreate the list.
  events = gcnew List<myoddweb::directorywatcher::interfaces::IEvent^>();
  for ( auto it = umEvents.begin(); it != umEvents.end(); ++it)
  {
    const auto e = (*it);
    auto event = gcnew Event();
    event->Name = gcnew System::String( e.Name.c_str());
    event->OldName = gcnew System::String( e.OldName.c_str());
    event->Action = (myoddweb::directorywatcher::interfaces::EventAction)e.Action;
    event->Error = (myoddweb::directorywatcher::interfaces::EventError)e.Error;
    event->DateTimeUtc = dateTime1970 + System::TimeSpan::FromMilliseconds( static_cast<double>(e.TimeMillisecondsUtc));
    event->IsFile = e.IsFile;
    events->Add(event);
  }
  return result;
}

