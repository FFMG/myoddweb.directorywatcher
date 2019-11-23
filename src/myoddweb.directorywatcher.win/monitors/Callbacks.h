#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    typedef int(__stdcall* EventCallback)(const wchar_t* text);
  }
}