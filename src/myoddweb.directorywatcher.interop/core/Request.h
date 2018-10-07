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
#include <string>

/**
 * \brief implementation of IRequest
 */
public ref class Request : myoddweb::directorywatcher::interfaces::IRequest
{
public:
  /**
   * \brief the path of the folder we will be monitoring
   */
  virtual property System::String^ Path;

  /**
   * \brief if we are recursively monitoring or not.
   */
  virtual property bool Recursive;
};

/**
 * \brief unmaned implementation of IRequest
 */
struct UmRequest
{
  /**
   * \brief the path of the folder we will be monitoring
   */
  std::wstring Path;

  /**
   * \brief if we are recursively monitoring or not.
   */
  bool Recursive;
};