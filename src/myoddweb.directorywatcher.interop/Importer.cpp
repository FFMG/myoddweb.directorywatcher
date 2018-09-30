#include "stdafx.h"
#include "Importer.h"
#include "CoreWatcher.h"

// --------------------------------------------------------------------------------------------------
extern "C" LPVOID WINAPI Importer(UINT32 id)
{
  switch (id)
  {
  case IID_IWatcher1:
    return new CoreWatcher();
    break;

  default:
    break;
  }
  return NULL;
}