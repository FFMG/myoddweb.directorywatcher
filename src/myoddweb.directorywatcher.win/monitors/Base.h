#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    /*
     * The number of ms we want to sleep between checks.
     * The longer we wait, the more time shutdown will take
     * The shorter we wait, the more CPU will be used.
     */
    #define MYODDWEB_SLEEP_BETWEEN_READS 200L

    /**
     * The maximum number of subpath we want to allow in multiple windows monitor.
     * If the number is too large the number of running threads will cause issues.
     */
    #define MYODDWEB_MAX_NUMBER_OF_SUBPATH 32L

    /***
     * The max number of levels we want to use.
     * The higher the number the higher number of thread
     * The smaller the number, the higher the rist of memory issues.
     */
    #define MYODDWEB_MAX_LEVEL_DEPTH 2
  }
}