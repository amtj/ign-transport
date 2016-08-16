/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#ifndef IGN_TRANSPORT_SUBSCRIPTIONHANDLER_HH_
#define IGN_TRANSPORT_SUBSCRIPTIONHANDLER_HH_

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "ignition/transport/Helpers.hh"
#include "ignition/transport/SubscribeOptions.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

namespace ignition
{
  namespace transport
  {
    /// \class ISubscriptionHandler SubscriptionHandler.hh
    /// ignition/transport/SubscriptionHandler.hh
    /// \brief Interface class used to manage generic protobub messages.
    class IGNITION_TRANSPORT_VISIBLE ISubscriptionHandler
    {
      /// \brief Constructor.
      /// \param[in] _nUuid UUID of the node registering the handler.
      public: explicit ISubscriptionHandler(const std::string &_nUuid,
                                            const SubscribeOptions &_opts)
        : hUuid(Uuid().ToString()),
          opts(_opts),
          nUuid(_nUuid)
      {
      }

      /// \brief Destructor.
      public: virtual ~ISubscriptionHandler()
      {
      }

      /// \brief Executes the local callback registered for this handler.
      /// \param[in] _msg Protobuf message received.
      /// \return True when success, false otherwise.
      public: virtual bool RunLocalCallback(
                                     const transport::ProtoMsg &_msg) const = 0;

      /// \brief Create a specific protobuf message given its serialized data.
      /// \param[in] _data The serialized data.
      /// \return Pointer to the specific protobuf message.
      public: virtual const std::shared_ptr<transport::ProtoMsg> CreateMsg(
        const std::string &_data) const = 0;

      /// \brief Get the type of the messages from which this subscriber
      /// handler is subscribed.
      /// \return String representation of the message type.
      public: virtual std::string TypeName() = 0;

      /// \brief Get the node UUID.
      /// \return The string representation of the node UUID.
      public: std::string NodeUuid() const
      {
        return this->nUuid;
      }

      /// \brief Get the unique UUID of this handler.
      /// \return A string representation of the handler UUID.
      public: std::string HandlerUuid() const
      {
        return this->hUuid;
      }

      /// \brief Unique handler's UUID.
      protected: std::string hUuid;

      /// \brief Subscribe options.
      protected: SubscribeOptions opts;

      /// \brief Node UUID.
      private: std::string nUuid;

      /// \brief Timestamp of the executed callback.
      private: std::chrono::time_point<std::chrono::steady_clock> cbTimestamp;
    };

    /// \class SubscriptionHandler SubscriptionHandler.hh
    /// \brief It creates a subscription handler for a specific protobuf
    /// message. 'T' is the Protobuf message type that will be used for this
    /// particular handler.
    template <typename T> class SubscriptionHandler
      : public ISubscriptionHandler
    {
      // Documentation inherited.
      public: explicit SubscriptionHandler(const std::string &_nUuid,
                                           SubscribeOptions *_opts)
        : ISubscriptionHandler(_nUuid, _opts)
      {
      }

      // Documentation inherited.
      public: const std::shared_ptr<transport::ProtoMsg> CreateMsg(
        const std::string &_data) const
      {
        // Instantiate a specific protobuf message
        auto msgPtr = std::make_shared<T>();

        // Create the message using some serialized data
        if (!msgPtr->ParseFromString(_data))
        {
          std::cerr << "SubscriptionHandler::CreateMsg() error: ParseFromString"
                    << " failed" << std::endl;
        }

        return msgPtr;
      }

      // Documentation inherited.
      public: std::string TypeName()
      {
        return T().GetTypeName();
      }

      /// \brief Set the callback for this handler.
      /// \param[in] _cb The callback with the following parameters:
      /// \param[in] _msg Protobuf message containing the topic update.
      public: void SetCallback(const std::function <void(const T &_msg)> &_cb)
      {
        this->cb = _cb;
      }

      // Documentation inherited.
      public: bool RunLocalCallback(const transport::ProtoMsg &_msg) const
      {
        int executedMsgsPerSec;

        // Check for subscribe options.
        if (this->opts)
        {
          int msgsPerSec = this->opts->SetMsgsPerSec();


          if (std::chrono::steady_clock::now() == cbTimestamp &&
              msgsPerSec <= executedMsgsPerSec++)
          {
            std::cerr << "SubscriptionHandler::RunLocalCallback() error: "
                      << "Callback limit exceeded" << std::endl;
            return false;
          }
        }

        // Execute the callback (if existing)
        if (this->cb)
        {
          auto msgPtr = google::protobuf::down_cast<const T*>(&_msg);

          this->cb(*msgPtr);

          cbTimestamp = std::chrono::steady_clock::now();
          executedMsgsPerSec++;

          return true;
        }
        else
        {
          std::cerr << "SubscriptionHandler::RunLocalCallback() error: "
                    << "Callback is NULL" << std::endl;
          return false;
        }
      }

      /// \brief Callback to the function registered for this handler with the
      /// following parameters:
      /// \param[in] _msg Protobuf message containing the topic update.
      private: std::function<void(const T &_msg)> cb;
    };
  }
}

#endif
