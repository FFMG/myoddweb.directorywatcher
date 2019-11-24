#pragma once
#include <chrono>
#include <thread>

namespace myoddweb
{
  namespace directorywatcher
  {
    class Timer
    {
    private:
      bool _mustStop;
      std::thread* _thread;

    public:
      Timer() : 
        _mustStop( false ),
        _thread(nullptr)
      {
      }
            
      ~Timer()
      {
        Stop();
      }

      template<typename Function>
      bool Start(Function function, const long long& delay)
      {
        Stop();
        _mustStop = false;
        _thread = new std::thread([=]()
          {
            for (;;)
            {
              if (_mustStop)
              {
                return;
              }
              std::this_thread::sleep_for(
                std::chrono::milliseconds( delay )
              );
              if (_mustStop)
              {
                return;
              }
              function();
            }
          });
        return true;
      }

      void Stop() 
      {
        _mustStop = true;
        if (_thread != nullptr && _thread->joinable() )
        {
          _thread->join();
        }

        delete _thread;
        _thread = nullptr;
      }
    };
  }
}
