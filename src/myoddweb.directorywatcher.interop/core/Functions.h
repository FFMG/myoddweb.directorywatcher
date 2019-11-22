// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "Request.h"
#include "Event.h"

/**
 * \brief Add a request to the watcher
 * \param the request we want to add.
 * \return -ve is error, +ve is unique identifier.
 */
typedef long long(__stdcall *f_Start)( const UmRequest&  );

/**
 * \brief remove a request 
 * \return success or not.
 */
typedef bool(__stdcall *f_Stop)(long long);

/**
 * \brief get the latest events for a given id.
 * \return the number of items or -ve if there was an error.
 */
typedef long long(__stdcall *f_GetEvents)(long long, std::vector<UmEvent>& );
