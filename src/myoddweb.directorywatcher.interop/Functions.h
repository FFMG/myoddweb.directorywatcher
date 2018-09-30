#pragma once

/**
 * Monitor a folder
 * const wchar* the path
 * bool recursive or not
 * return -ve is error, +ve is unique identifier.
 */
typedef __int64(__stdcall *f_Monitor)(const wchar_t*, bool );
