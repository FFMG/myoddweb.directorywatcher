#pragma once
#include "../myoddweb.directorywatcher.win/utils/Threads/Worker.h"
using myoddweb::directorywatcher::threads::Worker;

class TestWorker final : public ::Worker
{
public:
  const int _maxUpdate = 0;
  int _updateCalled = 0;
  int _startCalled = 0;
  int _endCalled = 0;
  int _stop = 0;

  TestWorker() = delete;

  explicit TestWorker(const int maxUpdate = 5) :
    _maxUpdate(maxUpdate)
  {
  }

  void OnWorkerStop() override { ++_stop; }
  bool OnWorkerStart() override
  {
    ++_startCalled;
    return true;
  }
  void OnWorkerEnd() override
  {
    ++_endCalled;
  }
  bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override
  {
    // we must have started
    EXPECT_TRUE(Started());
    if (_stop > 0)
    {
      return false;
    }
    return ++_updateCalled < _maxUpdate;
  }
};