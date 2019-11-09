// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

enum class FunctionTypes
{
  FunctionUnknown = 0,
  FunctionFirst = 1,
  FunctionStart = FunctionFirst,
  FunctionStop,
  FunctionGetEvents,
  FunctionLast
};