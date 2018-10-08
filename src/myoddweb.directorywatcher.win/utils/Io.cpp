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
     * \brief Combine 2 parts of a file into a single filename.
     *        we do not validate the path or even if the values make no sense.
     * \param lhs the left hand side
     * \param rhs the right hand side.
     * \return The 2 part connected togeter
     */
    std::wstring Io::Combine(const std::wstring& lhs, const std::wstring& rhs)
    {
      // sanity check, if the lhs.length is 0, then we just return the rhs.
      const auto s = lhs.length();
      if (s == 0)
      {
        return rhs;
      }

      // the two type of separators.
      const auto sep1 = L'/';
      const auto sep2 = L'\\';

      const auto l = lhs[s - 1];
      if (l != sep1 && l != sep2)
      {
#ifdef WIN32
        return lhs + sep2 + rhs;
#else
        return lhs + sep1 + rhs;
#endif
      }
      return lhs + rhs;
    }
  }
}