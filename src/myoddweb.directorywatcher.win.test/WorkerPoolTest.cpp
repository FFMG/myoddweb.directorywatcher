#include "../myoddweb.directorywatcher.win/utils/Threads/CallbackWorker.h"
#include "pch.h"
#include "../myoddweb.directorywatcher.win/utils/Threads/WorkerPool.h"
#include "../myoddweb.directorywatcher.win/utils/Threads/Worker.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"
#include "MonitorsManagerTestHelper.h"
#include "WorkerHelper.h"

using myoddweb::directorywatcher::threads::WorkerPool;
using myoddweb::directorywatcher::Wait;

TEST(WorkPool, DefaultValues) {
  {
    const auto pool = ::WorkerPool( 10 );
    EXPECT_FALSE( pool.started());
  }
  {
    const auto pool = ::WorkerPool(10);
    EXPECT_FALSE(pool.completed());
  }
}

TEST(WorkPool, StartIsCalledExactlyOnceWithQuickWorkers) {
  {
    auto worker1 = TestWorker(1);
    auto worker2 = TestWorker(1);
    auto pool = ::WorkerPool(10);
    pool.add(worker1);

    // give worker 1 a chance to complete
    auto wr = worker1.wait_for(100);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);

    // add another one
    pool.add(worker2);

    // then wait a bit for everything
    pool.wait_for(100);

    // surely start is called.
    EXPECT_EQ(1, worker1._startCalled);
    EXPECT_EQ(1, worker2._startCalled);

    // clean up
    wr = pool.stop_and_wait(100 );
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  }
}

TEST(WorkPool, StartIsCalledExactlyOnceEvenForCompleteWorker) {
  {
    // create a worker and run it.
    auto worker1 = TestWorker(1);
    worker1.execute();
    EXPECT_TRUE(worker1.completed());

    auto worker2 = TestWorker(1);
    auto pool = ::WorkerPool(10);

    // add it to the pool
    pool.add(worker1);

    // give worker 1 a chance to complete
    auto wr = worker1.wait_for(100);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);

    // add another one
    pool.add(worker2);

    // then wait a bit for everything
    pool.wait_for(100);

    // start should only be called once regaldess.
    EXPECT_EQ(1, worker1._startCalled);
    EXPECT_EQ(1, worker2._startCalled);

    // clean up
    wr = pool.stop_and_wait(100);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  }
}

TEST(WorkPool, EndIsCalledExactlyOnce) {
  {
    auto worker1 = TestWorker(1);
    auto worker2 = TestWorker(1);
    auto pool = ::WorkerPool(10);
    pool.add(worker1);
    pool.add(worker2);

  // wait for the pool to start
  if (!Wait::spin_until([&]
    {
      return pool.started();
    }, TEST_TIMEOUT_WAIT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

    // even if we wait a tiny bit, we still start
    pool.wait_for(100);

    // surely start is called.
    EXPECT_EQ(1, worker1._endCalled);
    EXPECT_EQ(1, worker2._endCalled);
  }
}

TEST(WorkPool, NumberOfTimesUpdatesIsCalled) {
  {
    for (auto i = 0; i < 10; ++i)
    {
      // we do not want this to be too quick
      // otherwise those might finish before we even start
      const auto numTimesWorker1 = 50;
      const auto numTimesWorker2 = 60;
      const auto pollTime = 10;

      auto worker1 = TestWorker(numTimesWorker1);
      auto worker2 = TestWorker(numTimesWorker2);

      auto pool = ::WorkerPool(pollTime);
      pool.add(worker1);
      pool.add(worker2);

      // wait for the pool to start
      if (!Wait::spin_until([&pool]
        {
          return pool.started();
        }, TEST_TIMEOUT_WAIT))
      {
        GTEST_FATAL_FAILURE_("Unable to start pool");
      }

      // we are not going to stop it
      // we just waiting for it to complete.
      const auto status = pool.wait_for( (numTimesWorker1 + numTimesWorker2) * TEST_TIMEOUT_WAIT );

      EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, status);
      EXPECT_EQ(worker1._maxUpdate, worker1._updateCalled);
      EXPECT_EQ(worker2._maxUpdate, worker2._updateCalled);

      // santy check that our test class is saving the correct values
      EXPECT_EQ(numTimesWorker1, worker1._updateCalled);
      EXPECT_EQ(numTimesWorker2, worker2._updateCalled);
    }
  }
}

TEST(WorkPool, WaitingForAWorkerThatIsNotOurs)
{
  auto worker1 = TestWorker(5);
  auto worker2 = TestWorker(1);

  auto pool = ::WorkerPool(10);
  pool.add(worker1);

  // wait for the pool to start
  if (!Wait::spin_until([&]
    {
      return pool.started();
    }, TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  // we are not going to stop it
  // we just waiting for it to complete.
  const auto status = pool.wait_for(worker2, 10000);

  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, status);
  EXPECT_EQ(1, worker1._startCalled);

  // but our start was never called.
  EXPECT_EQ(0, worker2._startCalled);
}

TEST(WorkPool, WaitUntiWhenNoWorker ) {
  {
    auto pool = ::WorkerPool(10);

    // we are not going to stop it
    // we just waiting for it to complete.
    const auto status = pool.wait_for(10000);

    // still complete.
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, status);
  }
}

TEST(WorkPool, WaitForASingleItem) {
  {
    auto worker1 = TestWorker(3);

    auto pool = ::WorkerPool(10);
    pool.add(worker1);

    // we are not going to stop it
    // we just waiting for it to complete.
    const auto status = pool.wait_for( worker1, 10000);

    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, status);
    EXPECT_EQ(worker1._maxUpdate, worker1._updateCalled);
  }
}

TEST(WorkPool, StopAndWait) {
  {
    for (auto i = 0; i < 10; ++i)
    {
      // make it very large number
      auto worker1 = TestWorker(5000);
      auto worker2 = TestWorker(6000);

      auto pool = ::WorkerPool(10);
      pool.add(worker1);
      pool.add(worker2);

      // wait for the pool to start
      if (!Wait::spin_until([&]
        {
          return pool.started();
        }, TEST_TIMEOUT_WAIT))
      {
        GTEST_FATAL_FAILURE_("Unable to start pool");
      }

      // run a little
      pool.wait_for(100);

      // we are not going to stop it
      // we just waiting for it to complete.
      const auto status = pool.stop_and_wait(1000);

      EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, status);
      EXPECT_TRUE(worker1._updateCalled <= worker1._maxUpdate);
      EXPECT_TRUE(worker2._updateCalled <= worker2._maxUpdate);

      EXPECT_TRUE(worker1._updateCalled > 0);
      EXPECT_TRUE(worker2._updateCalled > 0);

      // but we must have started
      EXPECT_EQ(1, worker1._startCalled);
      EXPECT_EQ(1, worker2._startCalled);
      EXPECT_EQ(1, worker1._endCalled);
      EXPECT_EQ(1, worker2._endCalled);
    }
  }
}

TEST(WorkPool, CheckHasStartedWithShortRunningWorker)
{
  for (auto i = 0; i < 10; ++i)
  {
    // create a worker that will run veru quick
    auto worker = TestWorker(1);

    // create a pool
    auto pool = ::WorkerPool(10);
    pool.add(worker);

    // the act of adding a worker could start it.
    // so we cannot test "pool.started()"

    // make sure that start by itself.
    if (!Wait::spin_until([&]
      {
        return pool.started() || pool.completed();
      }, TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to start pool");
    }

    // then wait for it to end.
    if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.stop_and_wait(2*TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to complete pool");
    }

    EXPECT_FALSE(pool.started());
  }
}

TEST(WorkPool, CheckHasStartedWithLongRunningWorker)
{
  for (auto i = 0; i < 10; ++i)
  {
    // create a worker that will run for a while
    auto worker = TestWorker(100);

    // create a pool
    auto pool = ::WorkerPool(10);
    pool.add(worker);

    // the act of adding a worker could start it.
    // so we cannot test "pool.started()"

    // make sure that start by itself.
    if (!Wait::spin_until([&]
      {
        return pool.started() || pool.completed();
      }, TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to start pool");
    }

    // wait a bit
    pool.wait_for(TEST_TIMEOUT_WAIT);

    // then wait for it to end.
    if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.stop_and_wait( TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to complete pool");
    }

    EXPECT_FALSE(pool.started());
  }
}

TEST(WorkPool, SingleCallbackWorkerEndsAsExpected)
{
  auto pool = ::WorkerPool(10);

  auto wasCalled = false;
  // then create a somple CallbackWorker
  auto cbWorker = myoddweb::directorywatcher::threads::CallbackWorker([&wasCalled]
    {
      wasCalled = true;
    });

  // then wait for our simple worker to complete.
  pool.add(cbWorker);

  // wait a little
  if (!Wait::spin_until([&]
    {
      return pool.started();
    }, TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  // then wait a couple of milliseconds
  // it should be really quick.
  const auto wr = pool.wait_for(cbWorker, TEST_TIMEOUT_WAIT);

  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  EXPECT_TRUE(wasCalled);

  // but we must be complete as well
  EXPECT_TRUE(cbWorker.completed());

  // just complete the test.
  if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.stop_and_wait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to complete pool");
  }

  EXPECT_FALSE(pool.started());
}

TEST(WorkPool, SingleCallbackWorkerWithOtherWorkerEndsAsExpected)
{
  // wait a while
  auto worker = TestWorker(5000);

  auto pool = ::WorkerPool(10);
  pool.add(worker);

  // wait a little
  if (!Wait::spin_until([&]
    {
      return pool.started();
    }, TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  auto wasCalled = false;
  // then create a somple CallbackWorker
  auto cbWorker = myoddweb::directorywatcher::threads::CallbackWorker([&wasCalled]
    {
      wasCalled = true;
    });

  // then wait for our simple worker to complete.
  pool.add(cbWorker);

  // then wait a couple of milliseconds
  // it should be really quick.
  const auto wr = pool.wait_for(cbWorker, TEST_TIMEOUT_WAIT);

  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  EXPECT_TRUE(wasCalled);

  // but we must be complete as well
  EXPECT_TRUE(cbWorker.completed());

  // just complete the test.
  if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.stop_and_wait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to complete pool");
  }

  EXPECT_FALSE(pool.started());
}

TEST(WorkPool, BlockingMultipleCallbackWorkerEndsAsExpected)
{
  // create a worker that has 200 events, (every 10ms = ~2000ms)
  auto worker = TestWorker(200);

  // add it to our work pool
  auto pool = ::WorkerPool(10);
  pool.add(worker);

  // wait for it to start
  if (!Wait::spin_until([&]
    {
      return pool.started();
    }, TEST_TIMEOUT_WAIT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  // the flag to check that it was called

  auto wasCalled = false;
  // then create a somple CallbackWorker that return very fast
  auto cbWorker = myoddweb::directorywatcher::threads::CallbackWorker([&wasCalled]
    {
      wasCalled = true;
    });

  // create another that is mostly blocking
  myoddweb::directorywatcher::threads::CallbackWorker* cbWorkerLong = nullptr;
  cbWorkerLong = new myoddweb::directorywatcher::threads::CallbackWorker([&cbWorkerLong]
    {
      for (;;)
      {
        if (cbWorkerLong->must_stop())
        {
          return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_TIMEOUT/2));
      }
    });

  // then wait for our simple worker to complete.
  pool.add(cbWorker);
  pool.add(*cbWorkerLong);

  // then wait for the quick worker to complete it should finish quick
  // if it blocks it means there is an error on our other, blocking function
  // that is preventing _this_ function from even running.
  const auto wr = pool.wait_for(cbWorker, TEST_TIMEOUT_WAIT );

  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  EXPECT_TRUE( wasCalled);

  // but we must be complete as well
  EXPECT_TRUE(cbWorker.completed() );

  // not call stop so the other one should also complete
  if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.stop_and_wait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to complete pool");
  }

  EXPECT_FALSE(pool.started());
  delete cbWorkerLong;
}

TEST(WorkPool, StoppingWorkpoolWhenAFunctionNeverEnds)
{
  // a worker that will work for 50*10 ms = ~500ms
  auto worker = TestWorker(50);
  auto pool = ::WorkerPool(10);
  pool.add(worker);

  // wait for the pool to start
  if (!Wait::spin_until([&]
    {
      return pool.started();
    }, TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  // this is the kill switch for this function so we can actually stop it.
  auto stop = false;

  // create a worker that has a blocking function
  auto cbWorkerLong = myoddweb::directorywatcher::threads::CallbackWorker([&stop]
    {
      // this will never end
      for (;;)
      {
        if( stop)
        {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_TIMEOUT / 2));
      }
    });

  // then wait for our simple worker to complete.
  pool.add(cbWorkerLong);

  // for it ... it will never end
  const auto wr = pool.wait_for(cbWorkerLong, TEST_TIMEOUT);
  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::timeout, wr);

  // try and complete the test, it will not happen as we have that one long running function
  if (myoddweb::directorywatcher::threads::WaitResult::complete == pool.stop_and_wait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("The pool should have timed out ... ");
  }

  // the pool cannot be complete as we have that one long running function.
  EXPECT_FALSE(pool.completed());

  // kill the function now.
  stop = true;
  pool.wait_for(cbWorkerLong, TEST_TIMEOUT_WAIT );

  if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.stop_and_wait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("The pool should have completed now ... ");
  }
}

TEST(WorkPool, AddAWorkerWithinAWorkerOnUpdate) {
  {
    // our pool and workers
    auto worker1 = TestWorker(5);
    auto pool = ::WorkerPool(10);

    // then create a simple CallbackWorker
    auto wasCalled = false;
    auto cbWorker = myoddweb::directorywatcher::threads::CallbackWorker([&]
      {
        wasCalled = true;

        // durring the update, add the other worker
        pool.add(worker1);
      });

    // then get things started
    // and add the callback worker.
    pool.add(cbWorker );

    // wait for the pool to start
    if (!Wait::spin_until([&]
      {
        return pool.started();
      }, TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to start pool");
    }

    // then wait for a couple of callback
    // every 10ms * 5 callback = ~50ms so 500 should be more than enough,
    const auto wr = pool.wait_for(500);

    // surely start is called.
    EXPECT_TRUE(wasCalled);
    EXPECT_EQ(1, worker1._endCalled);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  }
}

TEST(WorkPool, AddAWorkerWithinAWorkerOnStartDurringPoolStart) {
  {
    // our pool and workers
    auto worker1 = TestWorker(5);
    auto pool = ::WorkerPool(10);

    // then create a simple CallbackWorker
    auto cbWorker = TestWorkerOnStart(pool, worker1, 100 );
    auto cbWorker1 = TestWorkerOnStart(pool, cbWorker, 100);

    // then get things started
    // and add the callback worker.
    // because it is added now it will be called at the sametime as the pool "OnStart"
    pool.add(cbWorker1);

    // wait for the pool to start
    if (!Wait::spin_until([&]
      {
        return pool.started();
      }, TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to start pool");
    }

    // then wait for a couple of callback
    // every 10ms * 100 callback = ~1000ms
    const auto wr = pool.wait_for(10000);

    // surely start is called.
    EXPECT_EQ(1, worker1._endCalled);
    EXPECT_EQ(worker1._maxUpdate, worker1._updateCalled);
    EXPECT_EQ(cbWorker._maxUpdate, cbWorker._updateCalled);
    EXPECT_EQ(cbWorker1._maxUpdate, cbWorker1._updateCalled);
    EXPECT_EQ(1, cbWorker._endCalled);
    EXPECT_EQ(1, cbWorker1._endCalled);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  }
}

TEST(WorkPool, AddAWorkerWithinAWorkerOnStartDurringPoolUpdate) {
  {
    // have a long running worker
    auto worker1 = TestWorker(500);
    auto worker2 = TestWorker(500);
    auto pool = ::WorkerPool(10);

    // then create a simple CallbackWorker
    auto cbWorker = TestWorkerOnStart(pool, worker2, 100);

    // add the long running worker and make sure that everything started
    pool.add(worker1);

    // wait for the pool to start
    if (!Wait::spin_until([&]
      {
        return pool.started();
      }, TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to start pool");
    }

    // now that we started, add the worker that adds a worker
    // because it is added now it will be called at the sametime as the pool "OnUpdate"
    pool.add(cbWorker);

    // then wait for a couple of callback
    // every 10ms * 10000 callback = ~100000ms
    const auto wr = pool.wait_for(10000 + TEST_TIMEOUT_WAIT);

    // surely start is called.
    EXPECT_EQ(1, worker1._endCalled);
    EXPECT_EQ(worker1._maxUpdate, worker1._updateCalled);
    EXPECT_EQ(worker2._maxUpdate, worker2._updateCalled);
    EXPECT_EQ(cbWorker._maxUpdate, cbWorker._updateCalled);
    EXPECT_EQ(1, cbWorker._endCalled);
    EXPECT_EQ(1, worker1._endCalled);
    EXPECT_EQ(1, worker2._endCalled);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  }
}

