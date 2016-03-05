/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#ifndef __IGN_TRANSPORT_NODEOPTIONSPRIVATE_HH_INCLUDED__
#define __IGN_TRANSPORT_NODEOPTIONSPRIVATE_HH_INCLUDED__

#include <string>

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/NetUtils.hh"

namespace ignition
{
  namespace transport
  {
    /// \class NodeOptionsPrivate NodeOptionsPrivate.hh
    ///     ignition/transport/NodeOptionsPrivate.hh
    /// \brief Private data for Private NodeOption class.
    class IGNITION_VISIBLE NodeOptionsPrivate
    {
      /// \brief Constructor.
      public: NodeOptionsPrivate() = default;

      /// \brief Destructor.
      public: virtual ~NodeOptionsPrivate() = default;

      /// \brief Namespace for this node.
      public: std::string ns = "";

      /// \brief Partition for this node.
      public: std::string partition = hostname() + ":" + username();
    };
  }
}
#endif
