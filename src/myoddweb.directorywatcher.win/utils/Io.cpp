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
#include <windows.h>
#include "Io.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    Io::Io()
    {
    }

    Io::~Io()
    {
    }

    /**
     * \brief check if a given string is a file or a directory.
     * \param path the file we are checking.
     * \return if the string given is a file or not.
     */
    bool Io::IsFile( const std::wstring& path)
    {
      try
      {
        const auto attr = GetFileAttributesW(path.c_str());
        return ( attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
      }
      catch (...)
      {
        return false;
      }
    }

    /**
     * \brief combine 2 paths together making sure that the path is valid.
     * If both values are empty we return empty string
     * We assume that the lhs is a path and never a file, so we will add '\', (or '/' on *nix machines)
     * \param lhs the left hand side of the path
     * \param rhs the right hand side of the path
     * \return the combined path
     */
    std::wstring Io::Combine(const std::wstring& lhs, const std::wstring& rhs)
    {
      // sanity check, if the lhs.length is 0, then we just return the rhs.
      const auto sl = lhs.length();
      const auto sr = rhs.length();
      if (sl == 0 && sr == 0)
      {
        return L"";
      }

      // the two type of separators.
      const auto sep1 = L'/';
      const auto sep2 = L'\\';

      // and the one we will be using
#ifdef WIN32
      const auto sep = L'\\';
#else
      const auto sep = L'/';
#endif

      // we know that they are not both empty
      // if the right hand side is empty then the lhs cannot be
      if( sr == 0 )
      {
        const auto l = lhs[sl - 1];
        // no separator
        if (l != sep1 && l != sep2)
        {
          return lhs + sep;
        }
        // just go back one step
        return Combine(lhs.substr(0, sl - 1), rhs );
      }

      // we know that they are not both empty
      // if the left hand side is empty then the rhs cannot be
      if (sl == 0)
      {
        const auto r = rhs[0];
        // no separator
        if (r != sep1 && r != sep2)
        {
          return sep + rhs;
        }
        // just move forward one step
        return Combine(lhs, rhs.substr(1, sr - 1));
      }

      // if we are here, they both not zero
      const auto l = lhs[sl - 1];
      const auto r = rhs[0];

      // simple case where we have neither '/' not '\' on either sides.
      // neither of them has a back slash
      if (l != sep1 && l != sep2 && r != sep1 && r != sep2)
      {
        return lhs + sep + rhs;
      }

      // lhs does not have a back slash but the rhs does
      if (l != sep1 && l != sep2 && (r == sep1 || r == sep2))
      {
        return Combine( lhs, rhs.substr( 1, sr-1));
      }

      // rhs does not have a back slash but the lhs does
      if ((l == sep1 || l == sep2) && r != sep1 && r != sep2)
      {
        return Combine(lhs.substr(0, sl-1), rhs);
      }

      // if we are here, they both seem to have a backslash
      return Combine(lhs.substr(0, sl - 1), rhs.substr(1, sr - 1));
    }
  }
}