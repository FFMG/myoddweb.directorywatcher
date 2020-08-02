#include "pch.h"
#include "../myoddweb.directorywatcher.win/utils/Threads/Worker.h"
#include "../myoddweb.directorywatcher.win/utils/Wait.h"
#include "WorkerHelper.h"

TEST(Worker, DefaultValues)
{
  {
    const auto worker = TestWorker(1);
    EXPECT_FALSE(worker.Started() );
  }
  {
    const auto worker = TestWorker(1);
    EXPECT_FALSE(worker.Completed());
  }
}

TEST(Worker, StopWhatNeverStarted) {
  {
    auto worker = TestWorker(1);
    worker.Stop();
    EXPECT_TRUE(worker.Completed());
  }
}

TEST(Worker, StopAfterWeHaveStopped)
{
  auto worker = TestWorker(1);

  EXPECT_FALSE(worker.Started());
  EXPECT_FALSE(worker.Completed());
  
  worker.Execute();

  // if we are here, we are done.
  EXPECT_TRUE(worker.Completed());
  
  // and again
  worker.Stop();
  EXPECT_TRUE(worker.Completed());
}
