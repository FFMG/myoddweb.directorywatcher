// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <vector>

#include "../monitors/Base.h"
#include "../monitors/Callbacks.h"

namespace myoddweb:: directorywatcher
{
  class Logger final
  {
    /**
     * \brief Our list of loggers
     */
    std::vector<LoggerCallback> _loggers;

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
     * \param logger the logger we are adding.
     */
    void Add(const LoggerCallback& logger);

    /**
     * \brief log a message to a single logger
     * \param logger the logger we will be logging to
     * \param id owner the id
     * \param type the message type
     * \param format the message format
     * \param args the va_list of arguments
     */
    static void Log(const LoggerCallback& logger, long long id, int type, const wchar_t* format, va_list args);

    /**
     * \brief log a message to a single logger
     * \param logger the logger we will be logging to
     * \param id owner the id
     * \param type the message type
     * \param message the message we want to log.
     */
    static void Log(const LoggerCallback& logger, long long id, int type, const wchar_t* message );

    /**
     * \brief log a message to all our listed messages
     * \param id owner the id
     * \param type the message type
     * \param format the message format
     * \param ... the parametters
     */
    static void Log(long long id, int type, const wchar_t* format, ...);

  private:
    /**
     * \brief create a message, and take ownership of the string
     * \param format the message format
     * \param args the list of arguments.
     */
    [[nodiscard]]
    static wchar_t* MakeMessage(const wchar_t* format, va_list args);

    /**
     * \brief check if we have any loggers in our list
     */
    [[nodiscard]]
    bool HasAnyLoggers();
  };
}
