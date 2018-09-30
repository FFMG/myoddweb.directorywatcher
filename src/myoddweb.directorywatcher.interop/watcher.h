#pragma once
#include "CoreWatcher.h"
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

        /// <summary>
        /// The path we wish to monitor for changes
        /// </summary>
        /// <param name="path">The path we want to monitor.</param>
        /// <returns>Unique Id used to release/stop monitoring</returns>
        virtual __int64 Monitor(String^ path, bool recursive);

      protected:
        !Watcher();

      private:
        CoreWatcher* _coreWatcher;
      };
    }
  }
}
