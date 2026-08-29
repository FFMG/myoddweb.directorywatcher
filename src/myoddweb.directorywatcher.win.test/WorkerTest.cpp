#include "pch.h"
#include "../myoddweb.directorywatcher.win/utils/Threads/Worker.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"
#include "WorkerHelper.h"

TEST(Worker, DefaultValues)
{
  {
    const auto worker = TestWorker(1);
    EXPECT_FALSE(worker.started() );
  }
  {
    const auto worker = TestWorker(1);
    EXPECT_FALSE(worker.completed());
  }
}

TEST(Worker, StopWhatNeverStarted) {
  {
    auto worker = TestWorker(1);
    worker.stop();
    EXPECT_TRUE(worker.completed());
  }
}

TEST(Worker, StopAfterWeHaveStopped)
{
  auto worker = TestWorker(1);

  EXPECT_FALSE(worker.started());
  EXPECT_FALSE(worker.completed());
  
  worker.execute();

  // if we are here, we are done.
  EXPECT_TRUE(worker.completed());
  
  // and again
  worker.stop();
  EXPECT_TRUE(worker.completed());
}
