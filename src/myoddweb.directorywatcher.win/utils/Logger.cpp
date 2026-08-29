#include <stdarg.h>
#include "Logger.h"
#include "Lock.h"

namespace myoddweb::directorywatcher
{
  Logger Logger::_instance;
  MYODDWEB_MUTEX Logger::_lock;

  Logger::Logger() = default;

  Logger& Logger::instance()
  {
    return _instance;
  }

  /**
   * \brief add a logger to our list
   * \param id the id we are logging for.
   * \param logger the logger we are adding.
   */
  void Logger::add( const long long id, const LoggerCallback& logger)
  {
    if( nullptr == logger )
    {
      return;
    }
    MYODDWEB_LOCK(_lock);
    instance()._loggers[id] = logger;
  }

  /**
   * \brief remove a logger from the list.
   * \param id the id we are logging for.
   */
  void Logger::remove(const long long id)
  {
    MYODDWEB_LOCK(_lock);
    for(;;)
    {
      const auto logger = instance()._loggers.find(id);
      if( logger == instance()._loggers.end())
      {
        break;
      }
      instance()._loggers.erase(logger);
    }
  }

  /**
   * \brief log a message to all our listed messages
   * \param level the message log level
   * \param format the message format
   * \param ... the parametters
   */
  void Logger::log(const LogLevel level, const wchar_t* format, ...) noexcept
  {
    try
    {
      //  shortcut
      if (!has_any_loggers())
      {
        return;
      }

      va_list args;
      va_start(args, format);
      const auto message = make_message(format, args);
      va_end(args);

      MYODDWEB_LOCK(_lock);
      for (const auto& logger : instance()._loggers)
      {
        try
        {
          log(logger.second, 0, level, message.c_str());
        }
        catch (...)
        {
          // we cannot log a log message that faied
          MYODDWEB_OUT("There was an issue logging a message");
        }
      }
    }
    catch( ... )
    {
      /// we have a contract never to throw.
    }
  }

  /**
   * \brief log a message to all our listed messages
   * \param id owner the id
   * \param level the message log level
   * \param format the message format
   * \param ... the parametters
   */
  void Logger::log(const long long id, const LogLevel level, const wchar_t* format, ...) noexcept
  {
    try
    {
      //  shortcut
      if (!has_any_loggers())
      {
        return;
      }

      va_list args;
      va_start(args, format);
      const auto message = make_message(format, args);
      va_end(args);

      MYODDWEB_LOCK(_lock);
      if (id != 0)
      {
        const auto logger = instance()._loggers.find(id);
        if (logger != instance()._loggers.end())
        {
          try
          {
            log(logger->second, id, level, message.c_str());
          }
          catch (...)
          {
            // we cannot log a log message that faied
            MYODDWEB_OUT("There was an issue logging a message");
          }
        }
      }
      else
      {
        // the value was 0 so we will send to all.
        for (const auto& logger : instance()._loggers)
        {
          try
          {
            log(logger.second, id, level, message.c_str());
          }
          catch (...)
          {
            // we cannot log a log message that faied
            MYODDWEB_OUT("There was an issue logging a message");
          }
        }
      }
    }
    catch( ... )
    {
      // we have a contract never to throw
    }
  }

  /**
   * \brief log a message to a single logger
   * \param logger the logger we will be logging to
   * \param id owner the id
   * \param level the message log level
   * \param message the message we want to log.
   */
  void Logger::log(const LoggerCallback& logger, const long long id, const LogLevel level, const wchar_t* message) noexcept
  {
    if( nullptr == logger)
    {
      return;
    }

    try
    {
      logger
      (
        id,
        static_cast<int>(level),
        message
      );
    }
    catch( ... )
    {
      // we have a contract not to throw
    }
 }

  /**
   * \brief check if we have any loggers in our list
   */
  bool Logger::has_any_loggers() noexcept
  {
    MYODDWEB_LOCK(_lock);
    return !instance()._loggers.empty();
  }


  /**
   * \brief create a message, and take ownership of the string
   * \param format the message format
   * \param args the list of arguments.
   */
  std::wstring Logger::make_message(const wchar_t* format, const va_list args)noexcept
  {
    try
    {
      // sanity check
      if (nullptr == format)
      {
        return L"";
      }

      // get the final size
      const auto size = vswprintf(nullptr, 0, format, args);
      if (size <= 0)
      {
        return L"";
      }

      // build the string
      std::wstring output;
      const auto buffSize = size + 1;
      output.reserve(buffSize);
      if (vswprintf_s(output.data(), buffSize, format, args) < 0)// create the string
      {
        output.clear();                                     // Empty the string if there is a problem
      }
      return output;
    }
    catch (...)
    {
      // we have a contract not to throw
      return L"Exception thrown in `MakeMessage`!";
    }
  }

}
