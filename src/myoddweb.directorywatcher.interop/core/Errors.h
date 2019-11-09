// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

/**
 * Errors are negative, (or zero).
 */
enum Errors : __int64
{
  ErrorUnknown = 0,
  ErrorFolderNotFound = -1,
  ErrorFunctionNotFound = -2
};