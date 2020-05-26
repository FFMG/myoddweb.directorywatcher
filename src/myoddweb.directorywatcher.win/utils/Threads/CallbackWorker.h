// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "../../monitors/win/Data.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace threads
    {
      class CallbackWorker final : public Worker
      {
        TCallback _function;
      public:
        explicit CallbackWorker(TCallback function);

      protected:
        bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override;

        /**
         * \brief non blocking call to instruct the thread to stop.
         *        this derived class does not do anything so we will stop rightaway.
         */
        void Stop() override{}
      };
    }
  }
}