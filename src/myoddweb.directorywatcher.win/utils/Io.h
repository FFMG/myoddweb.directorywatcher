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

      /**
       * \brief combine 2 paths together making sure that the path is valid.
       * If both values are empty we return empty string
       * We assume that the lhs is a path and never a file, so we will add '\', (or '/' on *nix machines)
       * \param lhs the left hand side of the path
       * \param rhs the right hand side of the path
       * \return the combined path
       */
      static std::wstring Combine(const std::wstring& lhs, const std::wstring& rhs);

      /**
       * \brief check if a given string is a file or a directory.
       * \param path the file we are checking.
       * \return if the string given is a file or not.
       */
      static bool IsFile( const std::wstring& path);
    };
  }
}