// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <fstream>

#include <mutex>
#include <thread>

#define MYODDWEB_PROFILE_BUFFER 100

// from https://github.com/TheCherno/Hazel
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
    private:
      InstrumentationSession* m_CurrentSession;
      std::ofstream m_OutputStream;
      int m_ProfileCount;
      std::recursive_mutex _lock;
      std::vector<std::string> _msgs;
      const int _maxPacketSize;
    public:
      Instrumentor()
        : 
        m_CurrentSession(nullptr), 
        m_ProfileCount(0),
        _maxPacketSize(MYODDWEB_PROFILE_BUFFER)
      {
      }

      void BeginSession(const std::string& name, const std::string& filepath = "results.json")
      {
        m_OutputStream.open(filepath);
        WriteHeader();
        m_CurrentSession = new InstrumentationSession{ name };
      }

      void EndSession()
      {
        Flush();
        WriteFooter();
        m_OutputStream.close();
        delete m_CurrentSession;
        m_CurrentSession = nullptr;
        m_ProfileCount = 0;
      }

      void WriteProfile(const ProfileResult& result)
      {
        std::string name = result.Name;
        std::replace(name.begin(), name.end(), '"', '\'');

        std::stringstream ss;

        ss << "{";
        ss << "\"cat\":\"function\",";
        ss << "\"dur\":" << (result.End - result.Start) << ',';
        ss << "\"name\":\"" << name << "\",";
        ss << "\"ph\":\"X\",";
        ss << "\"pid\":0,";
        ss << "\"tid\":" << result.ThreadID << ',';
        ss << "\"ts\":" << result.Start;
        ss << "}";

        _lock.lock();
        try
        {
          _msgs.push_back(ss.str() );
          if (_msgs.size() > _maxPacketSize)
          {
            FlushInLock();
          }
        }
        catch (...)
        {
        }
        _lock.unlock();
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
        if (_msgs.size() == 0)
        {
          return;
        }
        for (auto it = _msgs.begin(); it != _msgs.end(); ++it)
        {
          if (m_ProfileCount++ > 0)
            m_OutputStream << ",";
          m_OutputStream << *it;
        }
        m_OutputStream.flush();
        _msgs.clear();
      }

      void WriteHeader()
      {
        m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
        m_OutputStream.flush();
      }

      void WriteFooter()
      {
        m_OutputStream << "]}";
        m_OutputStream.flush();
      }

      static Instrumentor& Get()
      {
        static Instrumentor instance;
        return instance;
      }
    };

    class InstrumentationTimer
    {
    public:
      InstrumentationTimer(const char* name)
        : m_Name(name), m_Stopped(false)
      {
        m_StartTimepoint = std::chrono::high_resolution_clock::now();
      }

      ~InstrumentationTimer()
      {
        if (!m_Stopped)
          Stop();
      }

      void Stop()
      {
        auto endTimepoint = std::chrono::high_resolution_clock::now();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
        Instrumentor::Get().WriteProfile({ m_Name, start, end, threadID });

        m_Stopped = true;
      }
    private:
      const char* m_Name;
      std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
      bool m_Stopped;
    };
  }
}

#ifdef _DEBUG
  #define MYODDWEB_PROFILE 1
#else
  #define MYODDWEB_PROFILE 0
#endif // DEBUG

#if MYODDWEB_PROFILE
#define MYODDWEB_PROFILE_BEGIN_SESSION(name, filepath) ::myoddweb::directorywatcher::Instrumentor::Get().BeginSession(name, filepath)
#define MYODDWEB_PROFILE_END_SESSION() ::myoddweb::directorywatcher::Instrumentor::Get().EndSession()
#define MYODDWEB_PROFILE_SCOPE(name) ::myoddweb::directorywatcher::InstrumentationTimer timer##__LINE__(name);
#define MYODDWEB_PROFILE_FUNCTION() MYODDWEB_PROFILE_SCOPE(__FUNCSIG__)
#else
#define MYODDWEB_PROFILE_BEGIN_SESSION(name, filepath)
#define MYODDWEB_PROFILE_END_SESSION()
#define MYODDWEB_PROFILE_SCOPE(name)
#define MYODDWEB_PROFILE_FUNCTION()
#endif 