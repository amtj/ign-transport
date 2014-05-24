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

#include <czmq.h>
#include <google/protobuf/message.h>
#include <algorithm>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "ignition/transport/Node.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;

//////////////////////////////////////////////////
transport::Node::Node(bool _verbose)
  : dataPtr(transport::NodePrivate::GetInstance(_verbose))
{
  uuid_generate(this->nodeUuid);
  this->nodeUuidStr = transport::GetGuidStr(this->nodeUuid);
}

//////////////////////////////////////////////////
transport::Node::~Node()
{
  for (auto topicInfo : this->dataPtr->topics.GetTopicsInfo())
  {
    zbeacon_t *topicBeacon = nullptr;
    if (this->dataPtr->topics.GetBeacon(topicInfo.first, &topicBeacon))
    {
      // Destroy the beacon.
      zbeacon_silence(topicBeacon);
      zbeacon_destroy(&topicBeacon);
      this->dataPtr->topics.SetBeacon(topicInfo.first, nullptr);
    }
  }

  // Unsubscribe from all the topics.
  for (auto topic : this->topicsSubscribed)
    this->Unsubscribe(topic);
}

//////////////////////////////////////////////////
void transport::Node::Advertise(const std::string &_topic)
{
  assert(_topic != "");

  zbeacon_t *topicBeacon = nullptr;

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  this->dataPtr->topics.SetAdvertisedByMe(_topic, true);

  if (!this->dataPtr->topics.GetBeacon(_topic, &topicBeacon))
  {
    // Create a new beacon for the topic.
    topicBeacon = zbeacon_new(this->dataPtr->ctx, this->dataPtr->bcastPort);
    this->dataPtr->topics.SetBeacon(_topic, topicBeacon);

    // Create the beacon content.
    transport::Header header(transport::Version, this->dataPtr->guid, _topic,
                             transport::AdvType, 0);
    transport::AdvMsg advMsg(header, this->dataPtr->myAddress,
                             this->dataPtr->myControlAddress);
    std::vector<char> buffer(advMsg.GetMsgLength());
    advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));

    zbeacon_set_interval(topicBeacon, 2000);
    zbeacon_noecho(topicBeacon);

    // Start publishing the ADVERTISE message periodically.
    zbeacon_publish(topicBeacon, reinterpret_cast<unsigned char*>(&buffer[0]),
                    advMsg.GetMsgLength());
  }
}

//////////////////////////////////////////////////
void transport::Node::Unadvertise(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  this->dataPtr->topics.SetAdvertisedByMe(_topic, false);

  // Stop broadcasting the beacon for this topic.
  zbeacon_t *topicBeacon = nullptr;
  if (this->dataPtr->topics.GetBeacon(_topic, &topicBeacon))
  {
    // Destroy the beacon.
    zbeacon_silence(topicBeacon);
    zbeacon_destroy(&topicBeacon);
    this->dataPtr->topics.SetBeacon(_topic, nullptr);
  }
}

//////////////////////////////////////////////////
int transport::Node::Publish(const std::string &_topic,
                             const transport::ProtoMsg &_msg)
{
  assert(_topic != "");

  if (!this->dataPtr->topics.AdvertisedByMe(_topic))
    return -1;

  // Local subscribers
  transport::ISubscriptionHandler_M handlers;
  this->dataPtr->topics.GetSubscriptionHandlers(_topic, handlers);
  for (auto handler : handlers)
  {
    transport::ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;
    if (subscriptionHandlerPtr)
      subscriptionHandlerPtr->RunLocalCallback(_topic, _msg);
    else
      std::cerr << "Subscription handler is NULL" << std::endl;
  }

  // Remote subscribers
  if (this->dataPtr->topics.HasRemoteSubscribers(_topic))
  {
    std::cout << "Sending data to remote subscribers" << std::endl;
    std::string data;
    _msg.SerializeToString(&data);
    if (this->dataPtr->Publish(_topic, data) != 0)
      return -1;
  }
  else
    std::cout << "There are no remote subscribers...SKIP" << std::endl;

  return 0;
}

//////////////////////////////////////////////////
void transport::Node::Unsubscribe(const std::string &_topic)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  if (this->dataPtr->verbose)
    std::cout << "\nUnsubscribe (" << _topic << ")\n";

  this->dataPtr->topics.RemoveSubscriptionHandler(_topic, this->nodeUuidStr);

  // Remove the topic from the list of subscribed topics in this node.
  this->topicsSubscribed.resize(
    std::remove(this->topicsSubscribed.begin(), this->topicsSubscribed.end(),
      _topic) - this->topicsSubscribed.begin());

  // Remove the filter for this topic if I am the last subscriber.
  if (!this->dataPtr->topics.Subscribed(_topic))
  {
    this->dataPtr->subscriber->setsockopt(
      ZMQ_UNSUBSCRIBE, _topic.data(), _topic.size());
  }

  // Notify the publisher
  transport::Addresses_M addresses;
  if (!this->dataPtr->topics.GetAdvAddresses(_topic, addresses))
  {
    std::cout << "Don't have information for topic [" << _topic
              << "]" << std::endl;
  }

  for (auto publisher : addresses)
  {
    std::string controlAddress = publisher.second;
    std::cout << "Unregistering address for topic [" << _topic << "]\n";
    std::cout << "\tAddress: [" << publisher.first << "]\n";
    std::cout << "\tNode: [" << this->nodeUuidStr << "]\n";

    zmq::socket_t socket(*this->dataPtr->context, ZMQ_DEALER);

    // Set ZMQ_LINGER to 0 means no linger period. Pending messages will be
    // discarded immediately when the socket is closed. That avoids infinite
    // waits if the publisher is disconnected.
    int lingerVal = 0;
    socket.setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));

    socket.connect(controlAddress.c_str());

    zmq::message_t message;
    message.rebuild(_topic.size() + 1);
    memcpy(message.data(), _topic.c_str(), _topic.size() + 1);
    socket.send(message, ZMQ_SNDMORE);

    message.rebuild(this->dataPtr->myAddress.size() + 1);
    memcpy(message.data(), this->dataPtr->myAddress.c_str(),
           this->dataPtr->myAddress.size() + 1);
    socket.send(message, ZMQ_SNDMORE);

    message.rebuild(this->nodeUuidStr.size() + 1);
    memcpy(message.data(), this->nodeUuidStr.c_str(),
           this->nodeUuidStr.size() + 1);
    socket.send(message, ZMQ_SNDMORE);

    std::string data = std::to_string(transport::EndConnection);
    message.rebuild(data.size() + 1);
    memcpy(message.data(), data.c_str(), data.size() + 1);
    socket.send(message, 0);
  }
}
