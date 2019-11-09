// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <Windows.h>
#include "Io.h"
#include  <cctype>

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief check if a given string is a file or a directory.
     * \param path the file we are checking.
     * \return if the string given is a file or not.
     */
    bool Io::IsFile(const std::wstring& path)
    {
      try
      {
        const auto cpath = path.c_str();
        const auto attr = GetFileAttributesW(cpath);
        if (attr != INVALID_FILE_ATTRIBUTES)
        {
          return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
        }

        const auto dw = GetLastError();
        if (ERROR_ACCESS_DENIED == dw)
        {
          WIN32_FIND_DATAW wfd = {};
          const auto handle = FindFirstFileW(cpath, &wfd);
          if (handle != INVALID_HANDLE_VALUE)
          {
            const auto attrfff = wfd.dwFileAttributes;
            FindClose(handle);
            return (attrfff & FILE_ATTRIBUTE_DIRECTORY) == 0;
          }
        }
        return true;
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
      if (sr == 0)
      {
        const auto l = lhs[sl - 1];
        // no separator
        if (l != sep1 && l != sep2)
        {
          return lhs + sep;
        }
        // just go back one step
        return Combine(lhs.substr(0, sl - 1), rhs);
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
        return Combine(lhs, rhs.substr(1, sr - 1));
      }

      // rhs does not have a back slash but the lhs does
      if ((l == sep1 || l == sep2) && r != sep1 && r != sep2)
      {
        return Combine(lhs.substr(0, sl - 1), rhs);
      }

      // if we are here, they both seem to have a backslash
      return Combine(lhs.substr(0, sl - 1), rhs.substr(1, sr - 1));
    }

    /**
     * \brief Check if a given directory is a dot or double dot
     * \param directory the lhs folder.
     * \return if it is a dot directory or not
     */
    bool Io::IsDot(const std::wstring& directory)
    {
      return directory == L"." || directory == L"..";
    }

    /**
     * \brief Get all the sub folders of a given folder.
     * \param folder the starting folder.
     * \return all the sub-folders, (if any).
     */
    std::vector<std::wstring> Io::GetAllSubFolders(const std::wstring& folder)
    {
      std::vector<std::wstring> subFolders;
      auto searchPath = Io::Combine(folder, L"/*.*");
      WIN32_FIND_DATA fd = {};
      const auto hFind = ::FindFirstFile(searchPath.c_str(), &fd);
      if (hFind != INVALID_HANDLE_VALUE)
      {
        do
        {
          // read all (real) files in current folder
          // , delete '!' read other 2 default folder . and ..
          if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
          {
            if (!Io::IsDot(fd.cFileName))
            {
              subFolders.emplace_back(Io::Combine(folder, fd.cFileName));
            }
          }
        } while (::FindNextFile(hFind, &fd));
        ::FindClose(hFind);
      }
      return subFolders;
    }

    std::wstring TidyFolderName(const std::wstring& lhs)
    {
#ifdef WIN32
      const auto sep = L'\\';
      const std::wstring ssep = L"\\\\";
      const auto badsep = L'/';
#else
      const auto sep = L'/';
      const std::wstring ssep = L"//";
      const auto badsep = L'\\';
#endif
      auto llhs = lhs;
      auto found = llhs.find_first_of(badsep);
      while (found != std::string::npos)
      {
        llhs[found] = sep;
        found = llhs.find_first_of(badsep);
      }

      const auto r = std::wstring() + sep;
      found = llhs.find( ssep, 0 );
      while (found != std::string::npos)
      {
        llhs.replace( found, 2, r );
        found = llhs.find(ssep, 0);
      }

      return llhs;
    }


    /**
     * \brief Compare if 2 folders are the same
     * \param lhs the first folder
     * \param rhs the second folder
     * \return if both folders are similar.
     */
    bool Io::AreSameFolders(const std::wstring& lhs, const std::wstring& rhs)
    {
#ifdef WIN32
      const auto sep = L'\\';
#else
      const auto sep = L'/';
#endif
      auto llhs = TidyFolderName(lhs);
      auto rrhs = TidyFolderName(rhs);

      auto sl = llhs.length();
      while( sl > 0 && (llhs[sl-1] == sep ))
      {
        llhs = llhs.substr(0, --sl );
      }

      auto sr = rrhs.length();
      while (sr > 0 && (rrhs[sr - 1] == sep))
      {
        rrhs = rrhs.substr(0, --sr);
      }

      // if the length is both zero
      // then I guess they are the same.
      if (sl == 0 && sr == 0 )
      {
        return true;
      }

      // if they are not the same length then they are not the same
      if (sl != sr )
      {
        return false;
      }

      for (auto i = 0; i < sl; ++i )
      {
        if (llhs[i] == rrhs[i] )
        {
          continue;
        }
        if( llhs[i] != std::tolower(rrhs[i]) )
        {
          return false;
        }
      }

      // if we are here, they are the same
      return true;
    }
  }
}