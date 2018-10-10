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
#pragma once
namespace myoddweb
{
  namespace directorywatcher
  {
    enum class EventError
    {
      /**
       * \brief No error
       */
      None = 0,

      /**
       * \brief General error
       */
      General = 1,

      /**
       * \brief General memory error, (out of and so on).
       */
      Memory = 2,

      /**
       * \brief there was an overflow.
       */
      Overflow = 3,

      /**
       * \brief the monitoring was stopped somehow.
       */
      Aborted = 4,

      /**
       * \brief Unable to even start the monitoring
       *        Is the path valid? Is the filename valid?
       */
      CannotStart = 5,

      /**
       * \brief Cannot access the file/folder
       */
      Access = 6,

      /**
       * \brief We did not have any file data
       */
      NoFileData = 7
    };
  }
}