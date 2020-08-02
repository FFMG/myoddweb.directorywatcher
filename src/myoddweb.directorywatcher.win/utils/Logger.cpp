#include <stdarg.h>
#include "Logger.h"
#include "Lock.h"

namespace myoddweb::directorywatcher
{
  Logger Logger::_instance;
  MYODDWEB_MUTEX Logger::_lock;

  Logger::Logger() = default;

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
   * \param level the message log level
   * \param format the message format
   * \param ... the parametters
   */
  void Logger::Log(const LogLevel level, const wchar_t* format, ...) noexcept
  {
    try
    {
      //  shortcut
      if (!HasAnyLoggers())
      {
        return;
      }

      va_list args;
      va_start(args, format);
      const auto message = MakeMessage(format, args);
      va_end(args);

      MYODDWEB_LOCK(_lock);
      for (const auto& logger : Instance()._loggers)
      {
        try
        {
          Log(logger.second, 0, level, message.c_str());
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
  void Logger::Log(const long long id, const LogLevel level, const wchar_t* format, ...) noexcept
  {
    try
    {
      //  shortcut
      if (!HasAnyLoggers())
      {
        return;
      }

      va_list args;
      va_start(args, format);
      const auto message = MakeMessage(format, args);
      va_end(args);

      MYODDWEB_LOCK(_lock);
      if (id != 0)
      {
        const auto logger = Instance()._loggers.find(id);
        if (logger != Instance()._loggers.end())
        {
          try
          {
            Log(logger->second, id, level, message.c_str());
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
        for (const auto& logger : Instance()._loggers)
        {
          try
          {
            Log(logger.second, id, level, message.c_str());
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
  void Logger::Log(const LoggerCallback& logger, const long long id, const LogLevel level, const wchar_t* message) noexcept
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
  bool Logger::HasAnyLoggers() noexcept
  {
    MYODDWEB_LOCK(_lock);
    return !Instance()._loggers.empty();
  }


  /**
   * \brief create a message, and take ownership of the string
   * \param format the message format
   * \param args the list of arguments.
   */
  std::wstring Logger::MakeMessage(const wchar_t* format, const va_list args)noexcept
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
