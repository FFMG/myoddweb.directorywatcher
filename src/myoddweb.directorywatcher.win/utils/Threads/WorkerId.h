#pragma once
#include "../../monitors/Base.h"

class WorkerId final
{
public:
  /// <summary>
  /// Get the next available id.
  /// </summary>
  /// <returns></returns>
  static long long NextId();

private:
  WorkerId();

  /// <summary>
  /// The current id number
  /// </summary>
  long long _id;

  /// <summary>
  /// The id lock
  /// </summary>
  MYODDWEB_MUTEX _idLock;

  /// <summary>
  /// Get the next id.
  /// </summary>
  /// <returns></returns>
  long long GetNextId();
};

