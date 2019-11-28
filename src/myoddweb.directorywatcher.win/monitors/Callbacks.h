#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief the callback function when an event is raised.
     */
    typedef int(__stdcall* EventCallback)(
      long long id,
      bool isFile,
      const wchar_t* name,
      const wchar_t* oldName,
      int action,
      int error,
      long long dateTimeUtc
      );
  }
}