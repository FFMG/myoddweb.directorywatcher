// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
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
