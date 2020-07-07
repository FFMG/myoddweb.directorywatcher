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
    EXPECT_FALSE( pool.Started());
  }
  {
    const auto pool = ::WorkerPool(10);
    EXPECT_FALSE(pool.Completed());
  }
}

TEST(WorkPool, StartIsCalledExactlyOnceWithQuickWorkers) {
  {
    auto worker1 = TestWorker(1);
    auto worker2 = TestWorker(1);
    auto pool = ::WorkerPool(10);
    pool.Add(worker1);

    // give worker 1 a chance to complete
    auto wr = worker1.WaitFor(100);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);

    // add another one
    pool.Add(worker2);

    // then wait a bit for everything
    pool.WaitFor(100);

    // surely start is called.
    EXPECT_EQ(1, worker1._startCalled);
    EXPECT_EQ(1, worker2._startCalled);

    // clean up
    wr = pool.StopAndWait(100 );
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  }
}

TEST(WorkPool, StartIsCalledExactlyOnceEvenForCompleteWorker) {
  {
    // create a worker and run it.
    auto worker1 = TestWorker(1);
    worker1.Execute();
    EXPECT_TRUE(worker1.Completed());

    auto worker2 = TestWorker(1);
    auto pool = ::WorkerPool(10);

    // add it to the pool
    pool.Add(worker1);

    // give worker 1 a chance to complete
    auto wr = worker1.WaitFor(100);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);

    // add another one
    pool.Add(worker2);

    // then wait a bit for everything
    pool.WaitFor(100);

    // start should only be called once regaldess.
    EXPECT_EQ(1, worker1._startCalled);
    EXPECT_EQ(1, worker2._startCalled);

    // clean up
    wr = pool.StopAndWait(100);
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  }
}

TEST(WorkPool, EndIsCalledExactlyOnce) {
  {
    auto worker1 = TestWorker(1);
    auto worker2 = TestWorker(1);
    auto pool = ::WorkerPool(10);
    pool.Add(worker1);
    pool.Add(worker2);

  // wait for the pool to start
  if (!Wait::SpinUntil([&]
    {
      return pool.Started();
    }, TEST_TIMEOUT_WAIT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

    // even if we wait a tiny bit, we still start
    pool.WaitFor(100);

    // surely start is called.
    EXPECT_EQ(1, worker1._endCalled);
    EXPECT_EQ(1, worker2._endCalled);
  }
}

TEST(WorkPool, NumberOfTimesUpdatesIsCalled) {
  {
    // we do not want this to be too quick
    // otherwise those might finish before we even start
    const auto numTimesWorker1 = 50;
    const auto numTimesWorker2 = 60;

    auto worker1 = TestWorker(numTimesWorker1);
    auto worker2 = TestWorker(numTimesWorker2);

    auto pool = ::WorkerPool(10);
    pool.Add( worker1 );
    pool.Add( worker2 );

    // wait for the pool to start
    if (!Wait::SpinUntil([&pool]
      {
        return pool.Started();
      }, TEST_TIMEOUT))
    {
      GTEST_FATAL_FAILURE_("Unable to start pool");
    }

    // we are not going to stop it
    // we just waiting for it to complete.
    const auto status = pool.WaitFor( (numTimesWorker1+numTimesWorker2)*TEST_TIMEOUT_WAIT );

    EXPECT_EQ( myoddweb::directorywatcher::threads::WaitResult::complete, status );
    EXPECT_EQ( worker1._maxUpdate, worker1._updateCalled );
    EXPECT_EQ( worker2._maxUpdate, worker2._updateCalled);

    // santy check that our test class is saving the correct values
    EXPECT_EQ(numTimesWorker1, worker1._updateCalled);
    EXPECT_EQ(numTimesWorker2, worker2._updateCalled);

  }
}

TEST(WorkPool, WaitingForAWorkerThatIsNotOurs)
{
  auto worker1 = TestWorker(5);
  auto worker2 = TestWorker(1);

  auto pool = ::WorkerPool(10);
  pool.Add(worker1);

  // wait for the pool to start
  if (!Wait::SpinUntil([&]
    {
      return pool.Started();
    }, TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  // we are not going to stop it
  // we just waiting for it to complete.
  const auto status = pool.WaitFor(worker2, 10000);

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
    const auto status = pool.WaitFor(10000);

    // still complete.
    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, status);
  }
}

TEST(WorkPool, WaitForASingleItem) {
  {
    auto worker1 = TestWorker(3);

    auto pool = ::WorkerPool(10);
    pool.Add(worker1);

    // we are not going to stop it
    // we just waiting for it to complete.
    const auto status = pool.WaitFor( worker1, 10000);

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
      pool.Add(worker1);
      pool.Add(worker2);

      // wait for the pool to start
      if (!Wait::SpinUntil([&]
        {
          return pool.Started();
        }, TEST_TIMEOUT_WAIT))
      {
        GTEST_FATAL_FAILURE_("Unable to start pool");
      }

      // run a little
      pool.WaitFor(100);

      // we are not going to stop it
      // we just waiting for it to complete.
      const auto status = pool.StopAndWait(1000);

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
    pool.Add(worker);

    // the act of adding a worker could start it.
    // so we cannot test "pool.Started()"

    // make sure that start by itself.
    if (!Wait::SpinUntil([&]
      {
        return pool.Started() || pool.Completed();
      }, TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to start pool");
    }

    // then wait for it to end.
    if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.StopAndWait(2*TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to complete pool");
    }

    EXPECT_FALSE(pool.Started());
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
    pool.Add(worker);

    // the act of adding a worker could start it.
    // so we cannot test "pool.Started()"

    // make sure that start by itself.
    if (!Wait::SpinUntil([&]
      {
        return pool.Started() || pool.Completed();
      }, TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to start pool");
    }

    // wait a bit
    pool.WaitFor(TEST_TIMEOUT_WAIT);

    // then wait for it to end.
    if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.StopAndWait( TEST_TIMEOUT_WAIT))
    {
      GTEST_FATAL_FAILURE_("Unable to complete pool");
    }

    EXPECT_FALSE(pool.Started());
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
  pool.Add(cbWorker);

  // wait a little
  if (!Wait::SpinUntil([&]
    {
      return pool.Started();
    }, TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  // then wait a couple of milliseconds
  // it should be really quick.
  const auto wr = pool.WaitFor(cbWorker, TEST_TIMEOUT_WAIT);

  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  EXPECT_TRUE(wasCalled);

  // but we must be complete as well
  EXPECT_TRUE(cbWorker.Completed());

  // just complete the test.
  if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.StopAndWait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to complete pool");
  }

  EXPECT_FALSE(pool.Started());
}

TEST(WorkPool, SingleCallbackWorkerWithOtherWorkerEndsAsExpected)
{
  // wait a while
  auto worker = TestWorker(5000);

  auto pool = ::WorkerPool(10);
  pool.Add(worker);

  // wait a little
  if (!Wait::SpinUntil([&]
    {
      return pool.Started();
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
  pool.Add(cbWorker);

  // then wait a couple of milliseconds
  // it should be really quick.
  const auto wr = pool.WaitFor(cbWorker, TEST_TIMEOUT_WAIT);

  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  EXPECT_TRUE(wasCalled);

  // but we must be complete as well
  EXPECT_TRUE(cbWorker.Completed());

  // just complete the test.
  if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.StopAndWait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to complete pool");
  }

  EXPECT_FALSE(pool.Started());
}

TEST(WorkPool, BlockingMultipleCallbackWorkerEndsAsExpected)
{
  // create a worker that has 200 events, (every 10ms = ~2000ms)
  auto worker = TestWorker(200);

  // add it to our work pool
  auto pool = ::WorkerPool(10);
  pool.Add(worker);

  // wait for it to start
  if (!Wait::SpinUntil([&]
    {
      return pool.Started();
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
        if (cbWorkerLong->MustStop())
        {
          return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_TIMEOUT/2));
      }
    });

  // then wait for our simple worker to complete.
  pool.Add(cbWorker);
  pool.Add(*cbWorkerLong);

  // then wait for the quick worker to complete it should finish quick
  // if it blocks it means there is an error on our other, blocking function
  // that is preventing _this_ function from even running.
  const auto wr = pool.WaitFor(cbWorker, TEST_TIMEOUT_WAIT );

  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, wr);
  EXPECT_TRUE( wasCalled);

  // but we must be complete as well
  EXPECT_TRUE(cbWorker.Completed() );

  // not call stop so the other one should also complete
  if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.StopAndWait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to complete pool");
  }

  EXPECT_FALSE(pool.Started());
  delete cbWorkerLong;
}

TEST(WorkPool, StoppingWorkpoolWhenAFunctionNeverEnds)
{
  // a worker that will work for 50*10 ms = ~500ms
  auto worker = TestWorker(50);
  auto pool = ::WorkerPool(10);
  pool.Add(worker);

  // wait for the pool to start
  if (!Wait::SpinUntil([&]
    {
      return pool.Started();
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
  pool.Add(cbWorkerLong);

  // for it ... it will never end
  const auto wr = pool.WaitFor(cbWorkerLong, TEST_TIMEOUT);
  EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::timeout, wr);

  // try and complete the test, it will not happen as we have that one long running function
  if (myoddweb::directorywatcher::threads::WaitResult::complete == pool.StopAndWait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("The pool should have timed out ... ");
  }

  // the pool cannot be complete as we have that one long running function.
  EXPECT_FALSE(pool.Completed());

  // kill the function now.
  stop = true;
  pool.WaitFor(cbWorkerLong, TEST_TIMEOUT_WAIT );

  if (myoddweb::directorywatcher::threads::WaitResult::complete != pool.StopAndWait(TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("The pool should have completed now ... ");
  }
}