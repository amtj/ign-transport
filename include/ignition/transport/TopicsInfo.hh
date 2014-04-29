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

#ifndef __IGN_TRANSPORT_TOPICSINFO_HH_INCLUDED__
#define __IGN_TRANSPORT_TOPICSINFO_HH_INCLUDED__

#include <google/protobuf/message.h>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    // Info about a topic for pub/sub
    class TopicInfo
    {
      /// \brief Constructor.
      public: TopicInfo();

      /// \brief Destructor.
      public: virtual ~TopicInfo();

      /// \brief List of addresses known for a topic.
      public: Topics_L addresses;

      /// \brief Am I connected to the topic?
      public: bool connected;

      /// \brief Am I advertising this topic?
      public: bool advertisedByMe;

      /// brief Am I subscribed to the topic?
      public: bool subscribed;

      /// brief Callback that will be executed in case of receiving new data.
      public: Callback cb;

      public: CallbackLocal_V localCallbacks;

      /// brief Is a service call pending?
      public: bool requested;

      /// brief Callback to handle service calls requested by other nodes.
      public: ReqCallback reqCb;

      /// brief Callback to manage the service call's response requested by me.
      public: RepCallback repCb;

      /// brief List that stores the pending service call requests. Every
      /// element of the list contains the serialized parameters for each
      /// request.
      public: std::list<std::string> pendingReqs;

      public: unsigned int numSubscribers;

      public: std::shared_ptr<transport::ISubscriptionHandler>
                 subscriptionHandler;
    };

    class TopicsInfo
    {
      /// \brief Constructor.
      public: TopicsInfo();

      /// \brief Destructor.
      public: virtual ~TopicsInfo();

      /// \brief Return if there is some information about a topic stored.
      /// \param[in] _topic Topic name.
      /// \return true If there is information about the topic.
      public: bool HasTopic(const std::string &_topic);

      /// \brief Get the known list of addresses associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[out] _addresses List of addresses
      /// \return true when we have info about this topic.
      public: bool GetAdvAddresses(const std::string &_topic,
                                   std::vector<std::string> &_addresses);

      /// \brief Return if an address is registered associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _address Address to check.
      /// \return true if the address is registered with the topic.
      public: bool HasAdvAddress(const std::string &_topic,
                                 const std::string &_address);

      /// \brief Return true if we are connected to a node
      /// advertising the topic.
      /// \param[in] _topic Topic name.
      /// \return true if we are connected.
      public: bool Connected(const std::string &_topic);

      /// \brief Return true if we are subscribed to a node advertising
      /// the topic.
      /// \param[in] _topic Topic name.
      /// \return true if we are subscribed.
      public: bool Subscribed(const std::string &_topic);

      /// \brief Return true if I am advertising the topic.
      /// \param[in] _topic Topic name.
      /// \return true if the topic is advertised by me.
      public: bool AdvertisedByMe(const std::string &_topic);

      /// \brief Return true if I am requesting the service call associated to
      /// the topic.
      /// \param[in] _topic Topic name.
      /// \return true if the service call associated to the topic is requested.
      public: bool Requested(const std::string &_topic);

      /// \brief Get the callback associated to a topic subscription.
      /// \param[in] _topic Topic name.
      /// \param[out] A pointer to the function registered for a topic.
      /// \return true if there is a callback registered for the topic.
      public: bool GetCallback(const std::string &_topic,
                               transport::Callback &_cb);

      /// \brief Get the REQ callback associated to a topic subscription.
      /// \param[in] _topic Topic name.
      /// \param[out] A pointer to the REQ function registered for a topic.
      /// \return true if there is a REQ callback registered for the topic.
      public: bool GetReqCallback(const std::string &_topic,
                                  transport::ReqCallback &_cb);

      /// \brief Get the REP callback associated to a topic subscription.
      /// \param[in] _topic Topic name.
      /// \param[out] A pointer to the REP function registered for a topic.
      /// \return true if there is a REP callback registered for the topic.
      public: bool GetRepCallback(const std::string &_topic,
                                  transport::RepCallback &_cb);

      /// \brief Returns if there are any pending requests in the queue.
      /// \param[in] _topic Topic name.
      /// \return true if there is any pending request in the queue.
      public: bool PendingReqs(const std::string &_topic);

      /// \brief Add a new address associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _address New address
      public: void AddAdvAddress(const std::string &_topic,
                                 const std::string &_address);

      public: void AddLocalCallback(const std::string &_topic,
                                    const transport::CallbackLocal &_cb);

      public: bool HasLocalCallback(const std::string &_topic);

      public: int RunLocalCallbacks(const std::string &_topic,
        const std::shared_ptr<google::protobuf::Message> &_msgPtr);

      /// \brief Remove an address associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[int] _address Address to remove.
      public: void RemoveAdvAddress(const std::string &_topic,
                                    const std::string &_address);

      /// \brief Set a new connected value to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _value New value to be assigned to the connected member.
      public: void SetConnected(const std::string &_topic, const bool _value);

      /// \brief Set a new subscribed value to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _value New value to be assigned to the subscribed member.
      public: void SetSubscribed(const std::string &_topic, const bool _value);

      /// \brief Set a new service call request to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _value New value to be assigned to the requested member.
      public: void SetRequested(const std::string &_topic, const bool _value);

      /// \brief Set a new advertised value to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _value New value to be assigned in advertisedByMe member.
      public: void SetAdvertisedByMe(const std::string &_topic,
                                     const bool _value);

      /// \brief Set a new callback associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _cb New callback.
      public: void SetCallback(const std::string &_topic,
                               const transport::Callback &_cb);

      /// \brief Set a new REQ callback associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _cb New callback.
      public: void SetReqCallback(const std::string &_topic,
                                  const transport::ReqCallback &_cb);

      /// \brief Set a new REP callback associated to a given topic.
      /// \param[in] _topic Topic name.
      /// \param[in] _cb New callback.
      public: void SetRepCallback(const std::string &_topic,
                                  const transport::RepCallback &_cb);

      /// \brief Add a new service call request to the queue.
      /// \param[in] _topic Topic name.
      /// \param[in] _data Parameters of the request.
      public: void AddReq(const std::string &_topic, const std::string &_data);

      /// \brief Add a new service call request to the queue.
      /// \param[in] _topic Topic name.
      /// \param[out] _data Parameters of the request.
      /// \return true if a request was removed.
      public: bool DelReq(const std::string &_topic, std::string &_data);

      public: bool HasSubscribers(const std::string &_topic);

      public: void AddSubscriber(const std::string &_topic);

      public: std::shared_ptr<transport::ISubscriptionHandler>
              GetSubscriptionHandler(const std::string &_topic);

      public: void AddSubscriptionHandler(const std::string &_topic,
                          const std::shared_ptr<ISubscriptionHandler> &_msgPtr);

      /// \brief Get a reference to the topics map.
      /// \return Reference to the topic map.
      public: transport::Topics_M& GetTopicsInfo();

      // Hash with the topic/topicInfo information for pub/sub.
      private: transport::Topics_M topicsInfo;
    };
  }
}

#endif
