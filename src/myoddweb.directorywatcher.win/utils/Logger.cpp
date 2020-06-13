#include "Logger.h"

#include <stdarg.h>


#include "Lock.h"

namespace myoddweb::directorywatcher
{
  Logger Logger::_instance;
  MYODDWEB_MUTEX Logger::_lock;

  Logger::Logger()
  {
  }

  Logger& Logger::Instance()
  {
    return _instance;
  }

  /**
   * \brief add a logger to our list
   * \param id the id we are logging for.
   * \param logger the logger we are adding.
   */
  void Logger::Add( const long long id, const LoggerCallback& logger)
  {
    if( nullptr == logger )
    {
      return;
    }
    MYODDWEB_LOCK(_lock);
    Instance()._loggers[id] = logger;
  }

  /**
   * \brief remove a logger from the list.
   * \param id the id we are logging for.
   */
  void Logger::Remove(const long long id)
  {
    MYODDWEB_LOCK(_lock);
    for(;;)
    {
      const auto logger = Instance()._loggers.find(id);
      if( logger == Instance()._loggers.end())
      {
        break;
      }
      Instance()._loggers.erase(logger);
    }
  }

  /**
 * \brief log a message to all our listed messages
 * \param id owner the id
 * \param level the message log level
 * \param format the message format
 * \param ... the parametters
 */
  void Logger::Log(const long long id, const LogLevel level, const wchar_t* format, ...)
  {
    va_list args;
    va_start(args, format);
    const auto message = MakeMessage(format, args);
    va_end(args);

    // anything to do?
    if (nullptr == message)
    {
      return;
    }

    MYODDWEB_LOCK(_lock);
    for( const auto logger : Instance()._loggers )
    {
      if( 0 != id && id != logger.first )
      {
        continue;
      }
      Log(logger.second, id, level, message);
    }
    delete[] message;
  }

  /**
   * \brief log a message to a single logger
   * \param logger the logger we will be logging to
   * \param id owner the id
   * \param level the message log level
   * \param message the message we want to log.
   */
  void Logger::Log(const LoggerCallback& logger, const long long id, const LogLevel level, const wchar_t* message)
  {
    if( nullptr == logger)
    {
      return;
    }

    logger
    (
      id,
      static_cast<int>(level),
      message
    );
  }

  /**
   * \brief check if we have any loggers in our list
   */
  bool Logger::HasAnyLoggers()
  {
    MYODDWEB_LOCK(_lock);
    return Instance()._loggers.size() > 0;
  }


  /**
   * \brief create a message, and take ownership of the string
   * \param format the message format
   * \param args the list of arguments.
   */
  wchar_t* Logger::MakeMessage(const wchar_t* format, va_list args)
  {
    const auto size = vswprintf(nullptr, 0, format, args);
    if (size <= 0)
    {
      return nullptr;
    }

    const auto buffSize = size + 1;
    const auto buf = new wchar_t[buffSize];
    memset(buf, L'\0', buffSize);
    vswprintf(buf, size, format, args);
    return buf;
  }

}
