#pragma once
#include <string>

namespace myoddweb
{
  namespace directorywatcher
  {
    // Start watching a folder
    extern "C" { __declspec(dllexport) __int64 monitor( wchar_t path, bool recursive ); }
  }
}
