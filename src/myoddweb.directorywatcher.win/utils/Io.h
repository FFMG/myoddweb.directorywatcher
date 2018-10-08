#pragma once
#include <string>

namespace myoddweb
{
  namespace directorywatcher
  {
    class Io
    {
    public:
      Io();
      ~Io();

      static std::wstring Combine(const std::wstring& lhs, const std::wstring& rhs);
    };
  }
}