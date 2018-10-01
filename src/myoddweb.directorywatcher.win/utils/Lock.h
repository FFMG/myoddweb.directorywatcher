#pragma once

#include <mutex>

class Lock
{
public:
  Lock(std::recursive_mutex& lock);
  virtual ~Lock();

private:
  std::recursive_mutex& _lock;
};

