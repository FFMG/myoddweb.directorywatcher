// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "core/Watcher1.h"
using namespace System;
using namespace System::Collections::Generic;

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace Interop
    {
      public ref class Watcher : public myoddweb::directorywatcher::interfaces::IWatcher1
      {
      public:
        Watcher();
        virtual ~Watcher();

        /**
         * \inheritdoc
         */
        virtual long long Start(myoddweb::directorywatcher::interfaces::IRequest^ request);

        /**
         * \inheritdoc
         */
        virtual bool Stop( long long id);

        /**
         * \inheritdoc
         */
        virtual long long GetEvents(long long id, IList<myoddweb::directorywatcher::interfaces::IEvent^> ^% events );
      protected:
        !Watcher();

      private:
        Watcher1* _coreWatcher;
      };
    }
  }
}
