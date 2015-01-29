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

#ifndef __IGN_TRANSPORT_SERVICERESULT_PRIVATE_HH_INCLUDED__
#define __IGN_TRANSPORT_SERVICERESULT_PRIVATE_HH_INCLUDED__

#include <string>

namespace ignition
{
  namespace transport
  {
    /// \class ServiceResultPrivate ServiceResultPrivate.hh
    /// ignition/transport/ServiceResultPrivate.hh
    /// \brief Private data for the ServiceResult class.
    class IGNITION_VISIBLE ServiceResultPrivate
    {
      /// \brief Constructor.
      public: ServiceResultPrivate() = default;

      /// \brief Destructor.
      public: virtual ~ServiceResultPrivate() = default;

      /// \brief Result of the service request.
      public: Result_t returnCode = Result_t::Pending;

      /// \brief Stores the exception information (if any).
      public: std::string exceptionMsg;
    };
  }
}
#endif
