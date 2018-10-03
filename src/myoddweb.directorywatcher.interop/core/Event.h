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

/**
 * \brief implementation of IEvent
 */
public ref class Event : myoddweb::directorywatcher::interfaces::IEvent
{
public:
  /**
   * \brief the path of the folder that raised the event.
   */
  virtual property System::String^ Path;

  /**
   * \brief Extra information, mostly null, (for example rename)
   */
  virtual property System::String^ Extra;

  /**
   * \brief The action.
   */
  virtual property myoddweb::directorywatcher::interfaces::EventAction Action;
};

/**
 * \brief unmaned implementation of IEvent
 */
struct UmEvent
{
  /**
   * \brief the path of the folder that raised the event.
   */
  std::wstring Path;

  /**
   * \brief Extra information, mostly null, (for example rename)
   */
  std::wstring Extra;

  /**
   * \brief The action.
   */
  int Action;
};
