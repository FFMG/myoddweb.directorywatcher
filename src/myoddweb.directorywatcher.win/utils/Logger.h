// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <unordered_map>
#include "../monitors/Base.h"
#include "../monitors/Callbacks.h"

namespace myoddweb:: directorywatcher
{
  enum class LogLevel;

  class Logger final
  {
    /**
     * \brief Our list of loggers
     */
    std::unordered_map<long long, LoggerCallback> _loggers;

    /**
     * \brief the lock to ensure single access.
     */
    static MYODDWEB_MUTEX _lock;

    // the singleton
    static Logger _instance;
    static Logger& Instance();

    explicit Logger();

  public:
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    const Logger& operator=(const Logger&) = delete;
    const Logger& operator=(Logger&&) = delete;

    /**
     * \brief add a logger to our list
     * \param id the id we are logging for.
     * \param logger the logger we are adding.
     */
    static void Add( long long id, const LoggerCallback& logger);

    /**
     * \brief remove a logger from the list.
     * \param id the id we are logging for.
     */
    static void Remove(long long id );

    /**
     * \brief log a message to all our listed messages
     * \param id owner the id
     * \param level the message log level
     * \param format the message format
     * \param ... the parametters
     */
    static void Log(long long id, LogLevel level, const wchar_t* format, ...) noexcept;

    /**
     * \brief log a message to all our listed messages
     * \param level the message log level
     * \param format the message format
     * \param ... the parametters
     */
    static void Log(LogLevel level, const wchar_t* format, ...) noexcept;

  private:
    /**
     * \brief log a message to a single logger
     * \param logger the logger we will be logging to
     * \param id owner the id
     * \param level the message log level
     * \param message the message we want to log.
     */
    static void Log(const LoggerCallback& logger, long long id, LogLevel level, const wchar_t* message) noexcept;

    /**
     * \brief create a message, and take ownership of the string
     * \param format the message format
     * \param args the list of arguments.
     */
    [[nodiscard]]
    static std::wstring MakeMessage(const wchar_t* format, va_list args) noexcept;

    /**
     * \brief check if we have any loggers in our list
     */
    [[nodiscard]]
    static bool HasAnyLoggers() noexcept;
  };
}
