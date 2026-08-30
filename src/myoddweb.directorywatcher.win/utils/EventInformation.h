// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "EventAction.h"
#include "EventError.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Information about a file/folder event.
     */
    struct event_information final
    {
      event_information() :
        TimeMillisecondsUtc(0),
        Action(EventAction::Unknown),
        Error(EventError::None ),
        Name( nullptr ),
        OldName( nullptr ),
        IsFile( false )
      {
      }

      event_information(
        const long long timeMillisecondsUtc,
        const EventAction action,
        const EventError error,
        const wchar_t* name,
        const wchar_t* oldName,
        const bool isFile
      )
      : event_information()
      {
        assign(name, oldName, action, error, timeMillisecondsUtc, isFile);
      }

      ~event_information()
      {
        clear();
      }

      event_information(const event_information&) = delete;
      const event_information& operator=(const event_information&) = delete;

      /**
       * \brief the time in Ms when this event was recorded.
       */
      long long TimeMillisecondsUtc;

      /**
       * \brief the action we are recording
       */
      EventAction Action;

      /**
       * \brief the action we are recording
       */
      EventError Error;

      /**
       * \brief the filename/folder that was updated.
       */
      wchar_t* Name;

      /**
       * \brief the old name in the case of a rename.
       */
      wchar_t* OldName;

      /**
     * \brief Boolean if the update is a file or a directory.
       */
      bool IsFile;

    private:
      void assign(const wchar_t* name, const wchar_t* oldName, const EventAction action, const EventError error, const long long timeMillisecondsUtc, const bool isFile)
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
