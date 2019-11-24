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

      Event(const wchar_t* name, const wchar_t* oldName, int action, int error, long long timeMillisecondsUtc, bool isFile) :
        Event()
      {
        Assign(name, oldName, action, error, timeMillisecondsUtc, isFile);
      }

      Event(const Event& src) : 
        Event()
      {
        Assign(src);
      }

      const Event& operator=(const Event& src)
      {
        Assign(src);
        return *this;
      }

      ~Event()
      {
        ClearName();
        ClearOldName();
      }

      bool operator==(const Event& rhs) const
      {
        return false;
      }
    private:
      void Assign(const Event& src)
      {
        if (this == &src)
        {
          return;
        }
        Assign( src.Name, src.OldName, src.Action, src.Error, src.TimeMillisecondsUtc, src.IsFile );
      }

      void Assign(const wchar_t* name, const wchar_t* oldName, int action, int error, long long timeMillisecondsUtc, bool isFile)
      {
        Action = action;
        Error = error;
        TimeMillisecondsUtc = timeMillisecondsUtc;
        IsFile = isFile;

        ClearName();
        ClearOldName();

        if (name != nullptr)
        {
          auto l = wcslen(name);
          Name = new wchar_t[l + 1];
          wmemset(Name, L'\0', l + 1);
          wcscpy_s(Name, l + 1, name);
        }

        if (oldName != nullptr)
        {
          auto l = wcslen(oldName);
          OldName = new wchar_t[l + 1];
          wmemset(OldName, L'\0', l + 1);
          wcscpy_s(OldName, l + 1, oldName);
        }
      }

      void ClearName()
      {
        if (Name == nullptr)
        {
          return;
        }
        delete[] Name;
        Name = nullptr;
      }

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
        // get rid of the name
        ClearName();

        // copy one over the other
        if (OldName != nullptr)
        {
          auto l = wcslen(OldName);
          Name = new wchar_t[l + 1];
          wmemset(Name, L'\0', l + 1);
          wcscpy_s(Name, l + 1, OldName );
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
