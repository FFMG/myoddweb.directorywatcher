#pragma once
#include "../myoddweb.directorywatcher.win/utils/Threads/Worker.h"
#include "../myoddweb.directorywatcher.win/utils/Threads/WorkerPool.h"

using myoddweb::directorywatcher::threads::Worker;
using myoddweb::directorywatcher::threads::WorkerPool;

class TestWorker : public ::Worker
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

  void on_worker_stop() override { ++_stop; }
  virtual bool on_worker_start() override
  {
    ++_startCalled;
    return true;
  }
  void on_worker_end() override
  {
    ++_endCalled;
  }
  bool on_worker_update(float fElapsedTimeMilliseconds) override
  {
    // we must have started
    EXPECT_TRUE(started());
    if (_stop > 0)
    {
      return false;
    }
    return ++_updateCalled < _maxUpdate;
  }
};

class TestWorkerOnStart : public TestWorker
{
  ::Worker& _worker;
  ::WorkerPool& _pool;

public:
  TestWorkerOnStart(::WorkerPool& pool, ::Worker& worker, const int maxUpdate = 5)
   : TestWorker( maxUpdate ),
   _worker( worker ),
   _pool( pool)
  {
  }

  bool on_worker_start() override
  {
    _pool.add(_worker);
    return TestWorker::on_worker_start();
  }
};
