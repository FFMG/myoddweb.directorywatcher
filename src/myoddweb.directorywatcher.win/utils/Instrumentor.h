// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>
#include <sstream>
#include <chrono>
#include <fstream>
#include <mutex>
#include <iomanip>
#include <thread>

#include "../monitors/Base.h"

/**
 * \brief how often we want to flush log data to disk.
 *        the bigger the size, the more memory will be used.
 *        it is all lost when we crash ... but then again the json doc is corrupted by then.
 */
#define MYODDWEB_PROFILE_BUFFER 1000

// from https://github.com/TheCherno/Hazel
// go to chrome://tracing/
namespace myoddweb::directorywatcher
{
  using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

  struct ProfileResult
  {
    std::string Name;

    FloatingPointMicroseconds Start;
    std::chrono::microseconds ElapsedTime;
    std::thread::id ThreadID;
  };

  struct InstrumentationSession
  {
    std::string Name;
  };

  class Instrumentor
  {
  private:
    std::mutex m_Mutex;
    InstrumentationSession* m_CurrentSession;
    std::ofstream m_OutputStream;
  public:
    Instrumentor()
      : m_CurrentSession(nullptr)
    {
    }

    void BeginSession(const std::string& name, const std::string& filepath = "results.json")
    {
      std::lock_guard lock(m_Mutex);
      if (m_CurrentSession)
      {
        // If there is already a current session, then close it before beginning new one.
        // Subsequent profiling output meant for the original session will end up in the
        // newly opened session instead.  That's better than having badly formatted
        // profiling output.
        //MYODDWEB_OUT("Instrumentor::BeginSession('{0}') when session '{1}' already open.", name, m_CurrentSession->Name);
        InternalEndSession();
      }
      m_OutputStream.open(filepath);

      if (m_OutputStream.is_open())
      {
        m_CurrentSession = new InstrumentationSession({ name });
        WriteHeader();
      }
      else
      {
        //MYODDWEB_OUT(("Instrumentor could not open results file '{0}'.", filepath);
      }
    }

    void EndSession()
    {
      std::lock_guard lock(m_Mutex);
      InternalEndSession();
    }

    void WriteProfile(const ProfileResult& result)
    {
      std::stringstream json;

      json << std::setprecision(3) << std::fixed;
      json << ",{";
      json << "\"cat\":\"function\",";
      json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
      json << "\"name\":\"" << result.Name << "\",";
      json << "\"ph\":\"X\",";
      json << "\"pid\":0,";
      json << "\"tid\":" << result.ThreadID << ",";
      json << "\"ts\":" << result.Start.count();
      json << "}";

      std::lock_guard lock(m_Mutex);
      if (m_CurrentSession)
      {
        m_OutputStream << json.str();
        m_OutputStream.flush();
      }
    }

    static Instrumentor& Get()
    {
      static Instrumentor instance;
      return instance;
    }

  private:

    void WriteHeader()
    {
      m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
      m_OutputStream.flush();
    }

    void WriteFooter()
    {
      m_OutputStream << "]}";
      m_OutputStream.flush();
    }

    // Note: you must already own lock on m_Mutex before
    // calling InternalEndSession()
    void InternalEndSession()
    {
      if (m_CurrentSession)
      {
        WriteFooter();
        m_OutputStream.close();
        delete m_CurrentSession;
        m_CurrentSession = nullptr;
      }
    }

  };

  class InstrumentationTimer
  {
  public:
    InstrumentationTimer(const char* name)
      : m_Name(name), m_Stopped(false)
    {
      m_StartTimepoint = std::chrono::steady_clock::now();
    }

    ~InstrumentationTimer()
    {
      if (!m_Stopped)
        Stop();
    }

    void Stop()
    {
      auto endTimepoint = std::chrono::steady_clock::now();
      auto highResStart = FloatingPointMicroseconds{ m_StartTimepoint.time_since_epoch() };
      auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

      Instrumentor::Get().WriteProfile({ m_Name, highResStart, elapsedTime, std::this_thread::get_id() });

      m_Stopped = true;
    }
  private:
    const char* m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
    bool m_Stopped;
  };

  namespace InstrumentorUtils {

    template <size_t N>
    struct ChangeResult
    {
      char Data[N];
    };

    template <size_t N, size_t K>
    constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
    {
      ChangeResult<N> result = {};

      size_t srcIndex = 0;
      size_t dstIndex = 0;
      while (srcIndex < N)
      {
        size_t matchIndex = 0;
        while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
          matchIndex++;
        if (matchIndex == K - 1)
          srcIndex += matchIndex;
        result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
        srcIndex++;
      }
      return result;
    }
  }
}

#ifdef _DEBUG
 /**
   * \brief turn profiling on/off, uses a _lot_ of disc space!
   *        go to chrome://tracing/
   *        open Profile-Global.json
   */
  #define MYODDWEB_PROFILE 1
#else
  #define MYODDWEB_PROFILE 0
#endif // DEBUG

#if MYODDWEB_PROFILE
  #if !defined(_DEBUG)
    #error "You cannot use profiling in release mode!"
  #endif
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