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