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
        virtual ~CallbackWorker() = default;

        CallbackWorker(const CallbackWorker&) = delete;
        CallbackWorker(CallbackWorker&&) = delete;
        CallbackWorker&& operator=(CallbackWorker&&) = delete;
        const CallbackWorker& operator=(const CallbackWorker&) = delete;

      protected:
        bool OnWorkerUpdate(float fElapsedTimeMilliseconds) override;
      };
    }
  }
}