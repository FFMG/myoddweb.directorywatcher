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

TEST(WorkPool, StartIsCalledExactlyOnce) {
  {
    auto worker1 = TestWorker(1);
    auto worker2 = TestWorker(1);
    auto pool = ::WorkerPool(10);
    pool.Add(worker1);
    pool.Add(worker2);

    // even if we wait a tiny bit, we still start
    pool.WaitFor(100);

    // surely start is called.
    EXPECT_EQ(1, worker1._startCalled);
    EXPECT_EQ(1, worker2._startCalled);
  }
}

TEST(WorkPool, EndIsCalledExactlyOnce) {
  {
    auto worker1 = TestWorker(1);
    auto worker2 = TestWorker(1);
    auto pool = ::WorkerPool(10);
    pool.Add(worker1);
    pool.Add(worker2);

    // wait for the thread to get started
    Wait::Delay(TEST_TIMEOUT_WAIT);

    // even if we wait a tiny bit, we still start
    pool.WaitFor(100);

    // surely start is called.
    EXPECT_EQ(1, worker1._endCalled);
    EXPECT_EQ(1, worker2._endCalled);
  }
}

TEST(WorkPool, NumberOfTimesUpdatesIsCalled) {
  {
    auto worker1 = TestWorker(5);
    auto worker2 = TestWorker(6);

    auto pool = ::WorkerPool(10);
    pool.Add( worker1 );
    pool.Add( worker2 );

    // wait for the thread to get started
    Wait::Delay(TEST_TIMEOUT_WAIT);

    // we are not going to stop it
    // we just waiting for it to complete.
    const auto status = pool.WaitFor(10000);

    EXPECT_EQ( myoddweb::directorywatcher::threads::WaitResult::complete, status );
    EXPECT_EQ( worker1._maxUpdate, worker1._updateCalled );
    EXPECT_EQ( worker2._maxUpdate, worker2._updateCalled);
  }
}

TEST(WorkPool, WaitingForAWorkerThatIsNotOurs)
{
  auto worker1 = TestWorker(5);
  auto worker2 = TestWorker(1);

  auto pool = ::WorkerPool(10);
  pool.Add(worker1);

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
    // make it very large number
    auto worker1 = TestWorker(5000);
    auto worker2 = TestWorker(6000);

    auto pool = ::WorkerPool(10);
    pool.Add(worker1);
    pool.Add(worker2);

    // just make sure that the thread starts
    // by surrendering our thread.
    Wait::Delay(5);

    // we are not going to stop it
    // we just waiting for it to complete.
    const auto status = pool.StopAndWait(10000);

    EXPECT_EQ(myoddweb::directorywatcher::threads::WaitResult::complete, status);
    EXPECT_TRUE( worker1._updateCalled <= worker1._maxUpdate);
    EXPECT_TRUE( worker2._updateCalled <= worker2._maxUpdate);

    // but we must have started
    EXPECT_EQ(1, worker1._startCalled);
    EXPECT_EQ(1, worker2._startCalled);
    EXPECT_EQ(1, worker1._endCalled);
    EXPECT_EQ(1, worker2._endCalled);
  }
}

TEST(WorkPool, CheckHasStarted)
{
  auto worker1 = TestWorker(5);
  auto worker2 = TestWorker(1);

  auto pool = ::WorkerPool(10);
  pool.Add(worker1);

  // probably has not started
  EXPECT_FALSE(pool.Started());

  // wait a little
  if( !Wait::SpinUntil( [&]
  {
    return pool.Started();
  }, TEST_TIMEOUT))
  {
    GTEST_FATAL_FAILURE_("Unable to start pool");
  }

  // just complete the test.
  if( myoddweb::directorywatcher::threads::WaitResult::complete != pool.StopAndWait(TEST_TIMEOUT) )
  {
    GTEST_FATAL_FAILURE_("Unable to complete pool");
  }

  EXPECT_FALSE(pool.Started());
}
