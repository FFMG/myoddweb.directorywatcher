#pragma once
#include <string>

namespace myoddweb
{
  namespace directorywatcher
  {
    // Start watching a folder
    extern "C" { __declspec(dllexport) long start( std::wstring path, bool recursive ); }
  }
}
