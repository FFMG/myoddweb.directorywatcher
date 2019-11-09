// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <Windows.h>
#include "Importer.h"
#include "Watcher1.h"

// --------------------------------------------------------------------------------------------------
extern "C" LPVOID WINAPI Importer(UINT32 id)
{
  switch (id)
  {
  case IID_IWatcher1:
    return new Watcher1();

  default:
    break;
  }
  return NULL;
}