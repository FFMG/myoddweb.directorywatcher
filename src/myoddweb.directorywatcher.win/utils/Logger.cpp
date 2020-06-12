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
   * \param logger the logger we are adding.
   */
  void Logger::Add(const LoggerCallback& logger)
  {
    MYODDWEB_LOCK(_lock);
    _loggers.emplace_back(logger);
  }

  /**
 * \brief log a message to all our listed messages
 * \param id owner the id
 * \param type the message type
 * \param format the message format
 * \param ... the parametters
 */
  void Logger::Log(const long long id, const int type, const wchar_t* format, ...)
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
      Log(logger, id, type, message);
    }
    delete[] message;
  }

  /**
   * \brief log a message to a single logger
   * \param logger the logger we will be logging to
   * \param id owner the id
   * \param type the message type
   * \param format the message format
   * \param args the va_list of arguments
   */
  void Logger::Log(const LoggerCallback& logger, const long long id, int type, const wchar_t* format, va_list args)
  {
    if( nullptr == logger )
    {
      return;
    }

    const auto message = MakeMessage(format, args);
    if( nullptr == message )
    {
      return;
    }

    Log(logger, id, type, message);
    delete[] message;
  }

  /**
   * \brief log a message to a single logger
   * \param logger the logger we will be logging to
   * \param id owner the id
   * \param type the message type
   * \param message the message we want to log.
   */
  void Logger::Log(const LoggerCallback& logger, const long long id, const int type, const wchar_t* message)
  {
    if( nullptr == logger)
    {
      return;
    }

    logger
    (
      id,
      type,
      message
    );
  }

  /**
   * \brief check if we have any loggers in our list
   */
  bool Logger::HasAnyLoggers()
  {
    MYODDWEB_LOCK(_lock);
    return _loggers.size() > 0;
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
