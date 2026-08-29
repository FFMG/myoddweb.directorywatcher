// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief unmanaged implementation of IEvent
     */
    struct event final
    {
      event() :
        Name(nullptr),
        OldName(nullptr),
        Action(0),
        Error(0),
        TimeMillisecondsUtc(0),
        IsFile(false)
      {

      }

      event(const wchar_t* name, const wchar_t* oldName, const int action, const int error, const long long timeMillisecondsUtc, const bool isFile) :
        event()
      {
        assign(name, oldName, action, error, timeMillisecondsUtc, isFile);
      }

      ~event()
      {
        clear();
      }

      // prevent copy and move
      event(const event& src) = delete;
      event(event&& src) = delete;
      const event& operator=(const event& src) = delete;
      const event& operator=(event&& src) = delete;

      void move_old_name_to_name()
      {
        // get rid of the current name
        clear_name();

        // copy one over the other
        if (OldName != nullptr)
        {
          const auto len = wcslen(OldName);
          Name = new wchar_t[len + 1];
          wmemset(Name, L'\0', len + 1);
          wcscpy_s(Name, len + 1, OldName);
        }

        // we can get rid of the old name
        clear_old_name();
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

    private:
      void assign(const wchar_t* name, const wchar_t* oldName, const int action, const int error, const long long timeMillisecondsUtc, const bool isFile)
      {
        // clear the old values.
        clear();

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
      void clear()
      {
        clear_name();
        clear_old_name();
      }

      /**
       * \brief free the name memory
       */
      void clear_name()
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
      void clear_old_name()
      {
        if (OldName == nullptr)
        {
          return;
        }
        delete[] OldName;
        OldName = nullptr;
      }
    };
  }
}
