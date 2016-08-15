/*
 * Copyright (C) 2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef IGN_TRANSPORT_SUBSCRIBEOPTIONS_HH_
#define IGN_TRANSPORT_SUBSCRIBEOPTIONS_HH_

#include <memory>

#include "ignition/transport/Helpers.hh"

namespace ignition
{
  namespace transport
  {
    class SubscribeOptionsPrivate;

    /// \class SubscribeOptions SubscribeOptions.hh
    /// ignition/transport/SubscribeOptions.hh
    /// \brief A class to provide different options for a subscription.
    class IGNITION_TRANSPORT_VISIBLE SubscribeOptions
    {
      /// \brief Constructor.
      public: SubscribeOptions();

      /// \brief Copy constructor.
      /// \param[in] _newMsgsPerSec SubscribeOptions to copy.
      public: SubscribeOptions(const SubscribeOptions _newMsgsPerSec);

      /// \brief Destructor.
      public: ~SubscribeOptions();

      /// \brief Set the msgsPerSec of the topic/service.
      /// \param[in] _msgsPerSec to be set.
      public: void SetMsgsPerSec(int _newMsgsPerSec);

      /// \brief Get the msgsPerSec used in this topic/service.
      /// \return The msgsPerSec.
      public: int MsgsPerSec() const;

      /// \internal
      /// \brief Shared pointer to private data.
      protected: std::unique_ptr<SubscribeOptionsPrivate> dataPtr;
    };
  }
}
#endif
