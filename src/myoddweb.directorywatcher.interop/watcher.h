#pragma once

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

      protected:
        !Watcher();
      };
    }
  }
}
