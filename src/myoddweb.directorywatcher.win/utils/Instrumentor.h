// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <vector>
#include <mutex>

/**
 * \brief how often we want to flush log data to disk.
 *        the bigger the size, the more memory will be used.
 *        it is all lost when we crash ... but then again the json doc is corrupted by then.
 */
#define MYODDWEB_PROFILE_BUFFER 1000

// from https://github.com/TheCherno/Hazel
// go to chrome://tracing/
namespace myoddweb
{
  namespace directorywatcher
  {
    struct ProfileResult
    {
      std::string Name;
      long long Start, End;
      uint32_t ThreadID;
    };

    struct InstrumentationSession
    {
      std::string Name;
    };

    class Instrumentor
    {
      InstrumentationSession* _currentSession;
      std::ofstream _outputStream;
      uint32_t _profileCount;
      std::recursive_mutex _lock;
      std::vector<ProfileResult> _msgs;
      const uint32_t _maxPacketSize;
    public:
      Instrumentor()
        : 
        _currentSession(nullptr), 
        _profileCount(0),
        _maxPacketSize(MYODDWEB_PROFILE_BUFFER)
      {
      }

      void BeginSession(const std::string& name, const std::string& filepath = "results.json")
      {
        _outputStream.open(filepath);
        WriteHeader();
        _currentSession = new InstrumentationSession{ name };
      }

      void EndSession()
      {
        Flush();
        WriteFooter();
        _outputStream.close();
        delete _currentSession;
        _currentSession = nullptr;
        _profileCount = 0;
      }

      void WriteProfile(const ProfileResult& result)
      {
        _lock.lock();
        try
        {
          _msgs.emplace_back(result);
          if (static_cast<uint32_t>(_msgs.size()) > _maxPacketSize)
          {
            FlushInLock();
          }
        }
        catch (...)
        {
        }
        _lock.unlock();
      }

      static Instrumentor& Get()
      {
        static Instrumentor instance;
        return instance;
      }

    private:
      /**
       * \brief given the profile result create the json string to add.
       * \param result the data we want to build the string with
       * \return the created string.
       */
      static std::string BuildMessage(const ProfileResult& result)
      {
        auto name = result.Name;
        std::replace(name.begin(), name.end(), '"', '\'');

        std::stringstream ss;

        ss << "{";
        ss << R"("cat":"function",)";
        ss << "\"dur\":" << (result.End - result.Start) << ',';
        ss << R"("name":")" << name << "\",";
        ss << "\"ph\":\"X\",";
        ss << "\"pid\":0,";
        ss << "\"tid\":" << result.ThreadID << ',';
        ss << "\"ts\":" << result.Start;
        ss << "}";
        return ss.str();
      }

      void Flush()
      {
        _lock.lock();
        try
        {
          FlushInLock();
        }
        catch (...)
        {
        }
        _lock.unlock();
      }

      void FlushInLock()
      {
        if (_msgs.empty())
        {
          return;
        }
        for (auto it = _msgs.begin(); it != _msgs.end(); ++it)
        {
          if (_profileCount++ > 0)
          {
            _outputStream << ",";
          }
          _outputStream << BuildMessage( *it );
        }
        _outputStream.flush();
        _msgs.clear();
      }

      void WriteHeader()
      {
        _outputStream << "{\"otherData\": {},\"traceEvents\":[";
        _outputStream.flush();
      }

      void WriteFooter()
      {
        _outputStream << "]}";
        _outputStream.flush();
      }
    };

    class InstrumentationTimer
    {
      const char* _name;
      std::chrono::time_point<std::chrono::high_resolution_clock> _startTimepoint;
      bool _stopped;
      const uint32_t _threadId;

    public:
      InstrumentationTimer(const char* name, const uint32_t threadId)
        : 
        _name(name), 
        _stopped(false),
        _threadId( threadId )
      {
        _startTimepoint = std::chrono::high_resolution_clock::now();
      }

      ~InstrumentationTimer()
      {
        if (!_stopped)
        {
          Stop();
        }
      }

      void Stop()
      {
        const auto endTimepoint = std::chrono::high_resolution_clock::now();

        const auto start = std::chrono::time_point_cast<std::chrono::microseconds>(_startTimepoint).time_since_epoch().count();
        const auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        Instrumentor::Get().WriteProfile({ _name, start, end, _threadId });

        _stopped = true;
      }
    };
  }
}

#ifdef _DEBUG
 /**
   * \brief turn profiling on/off, uses a _lot_ of disc space!
   *        go to chrome://tracing/
   *        open Profile-Global.json
   */
  #define MYODDWEB_PROFILE 0
#else
  #define MYODDWEB_PROFILE 0
#endif // DEBUG

#if MYODDWEB_PROFILE
  #define MYODDWEB_PROFILE_BEGIN_SESSION(name, filepath) ::myoddweb::directorywatcher::Instrumentor::Get().BeginSession(name, filepath)
  #define MYODDWEB_PROFILE_END_SESSION() ::myoddweb::directorywatcher::Instrumentor::Get().EndSession()
  #define MYODDWEB_PROFILE_SCOPE(name, threadId) ::myoddweb::directorywatcher::InstrumentationTimer timer##__LINE__(name, threadId);
  #define MYODDWEB_PROFILE_FUNCTION_WITHTHREADID() MYODDWEB_PROFILE_SCOPE(__FUNCSIG__, static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())))
  #define MYODDWEB_PROFILE_FUNCTION() MYODDWEB_PROFILE_SCOPE(__FUNCSIG__, (uint32_t)0)
#else
  #define MYODDWEB_PROFILE_BEGIN_SESSION(name, filepath)
  #define MYODDWEB_PROFILE_END_SESSION()
  #define MYODDWEB_PROFILE_SCOPE(name)
  #define MYODDWEB_PROFILE_FUNCTION()
  #define MYODDWEB_PROFILE_FUNCTION_WITHTHREADID()
#endif 