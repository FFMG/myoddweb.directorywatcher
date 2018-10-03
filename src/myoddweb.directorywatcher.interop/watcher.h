//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
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
      protected:
        !Watcher();

      private:
        Watcher1* _coreWatcher;
      };
    }
  }
}
