// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief unmanaged implementation of IEvent
     */
    class Event
    {
    public:
      Event() : 
        Name(nullptr),
        OldName(nullptr),
        Action(0),
        Error(0),
        TimeMillisecondsUtc(0),
        IsFile(false)
      {

      }

      Event(const wchar_t* name, const wchar_t* oldName, const int action, const int error, const long long timeMillisecondsUtc, const bool isFile) :
        Event()
      {
        Assign(name, oldName, action, error, timeMillisecondsUtc, isFile);
      }

      ~Event()
      {
        Clear();
      }

      // prevent copy
      Event(const Event& src) = delete;
      const Event& operator=(const Event& src) = delete;

    private:
      void Assign(const wchar_t* name, const wchar_t* oldName, const int action, const int error, const long long timeMillisecondsUtc, const bool isFile)
      {
        // clear the old values.
        Clear();

        // and set the values.
        Action = action;
        Error = error;
        TimeMillisecondsUtc = timeMillisecondsUtc;
        IsFile = isFile;

        if (name != nullptr)
        {
          const auto len = wcslen(name);
          Name = new wchar_t[len + 1];
          wmemset(Name, L'\0', len + 1);
          wcscpy_s(Name, len + 1, name);
        }

        if (oldName != nullptr)
        {
          const auto len = wcslen(oldName);
          OldName = new wchar_t[len + 1];
          wmemset(OldName, L'\0', len + 1);
          wcscpy_s(OldName, len + 1, oldName);
        }
      }

      /**
       * \brief free all the memory
       */
      void Clear()
      {
        ClearName();
        ClearOldName();
      }

      /**
       * \brief free the name memory
       */
      void ClearName()
      {
        if (Name == nullptr)
        {
          return;
        }
        delete[] Name;
        Name = nullptr;
      }

      /**
       * \brief free the old name memory
       */
      void ClearOldName()
      {
        if (OldName == nullptr)
        {
          return;
        }
        delete[] OldName;
        OldName = nullptr;
      }

    public:
      void MoveOldNameToName()
      {
        // get rid of the current name
        ClearName();

        // copy one over the other
        if (OldName != nullptr)
        {
          const auto len = wcslen(OldName);
          Name = new wchar_t[len + 1];
          wmemset(Name, L'\0', len + 1);
          wcscpy_s(Name, len + 1, OldName );
        }

        // we can get rid of the old name
        ClearOldName();
      }

      /**
       * \brief The path that was changed.
       */
      wchar_t* Name;

      /**
       * \brief Extra information, (used for rename and so on).
       */
      wchar_t* OldName;

      /**
       * \brief the action.
       */
      int Action;

      /**
       * \brief the error.
       */
      int Error;

      /**
       * \brief when the event happened in ms
       */
      long long TimeMillisecondsUtc;

      /**
     * \brief Boolean if the update is a file or a directory.
       */
      bool IsFile;
    };
  }
}
