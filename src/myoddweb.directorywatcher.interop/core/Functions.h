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
#include "Request.h"

/**
 * \brief Add a request to the watcher
 * \param the request we want to add.
 * \return -ve is error, +ve is unique identifier.
 */
typedef long long(__stdcall *f_Start)( Request );

/**
 * \brief remove a request 
 * \return success or not.
 */
typedef bool(__stdcall *f_Stop)(long long);

/**
 * \brief the callback function.
 * \return success or not.
 */
typedef bool(__stdcall *f_callback)(std::wstring);

/**
 * \brief register a callback function.
 * \return the id of the registered function.
 */
typedef long long(__stdcall *f_Register)(long long, f_callback );
