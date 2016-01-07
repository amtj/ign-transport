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

#ifdef _MSC_VER
# pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>
#ifdef _MSC_VER
# pragma warning(pop)
#endif
#include "ignition/transport/Node.hh"
#include "ignition/transport/NodeOptions.hh"
#include "ignition/transport/NodePrivate.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/TopicUtils.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Node::Node(const NodeOptions &_options)
  : dataPtr(new NodePrivate())
{
  // Generate the node UUID.
  Uuid uuid;
  this->dataPtr->nUuid = uuid.ToString();

  // Save the options.
  this->dataPtr->options = _options;
}

//////////////////////////////////////////////////
Node::~Node()
{
  // Unsubscribe from all the topics.
  auto subsTopics = this->SubscribedTopics();
  for (auto const &topic : subsTopics)
    this->Unsubscribe(topic);

  // The list of subscribed topics should be empty.
  assert(this->SubscribedTopics().empty());

  // Unadvertise all my topics.
  auto advTopics = this->AdvertisedTopics();
  for (auto const &topic : advTopics)
  {
    if (!this->Unadvertise(topic))
    {
      std::cerr << "Node::~Node(): Error unadvertising topic ["
                << topic << "]" << std::endl;
    }
  }

  // The list of advertised topics should be empty.
  assert(this->AdvertisedTopics().empty());

  // Unadvertise all my services.
  auto advServices = this->AdvertisedServices();
  for (auto const &service : advServices)
  {
    if (!this->UnadvertiseSrv(service))
    {
      std::cerr << "Node::~Node(): Error unadvertising service ["
                << service << "]" << std::endl;
    }
  }

  // The list of advertised services should be empty.
  assert(this->AdvertisedServices().empty());
}

//////////////////////////////////////////////////
std::vector<std::string> Node::AdvertisedTopics() const
{
  std::vector<std::string> v;

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  for (auto topic : this->dataPtr->topicsAdvertised)
  {
    // Remove the partition information.
    topic.erase(0, topic.find_last_of("@") + 1);
    v.push_back(topic);
  }

  return v;
}

//////////////////////////////////////////////////
bool Node::Unadvertise(const std::string &_topic)
{
  std::string fullyQualifiedTopic = _topic;
  if (!TopicUtils::GetFullyQualifiedName(this->Options().Partition(),
    this->Options().NameSpace(), _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock(this->Shared()->discovery->Mutex(), this->dataPtr->shared->mutex);
  std::lock_guard<std::recursive_mutex> discLk(
    this->Shared()->discovery->Mutex(), std::adopt_lock);
  std::lock_guard<std::recursive_mutex> lk(
    this->dataPtr->shared->mutex, std::adopt_lock);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->topicsAdvertised.erase(fullyQualifiedTopic);

  // Notify the discovery service to unregister and unadvertise my topic.
  if (!this->dataPtr->shared->discovery->UnadvertiseMsg(fullyQualifiedTopic,
    this->dataPtr->nUuid))
  {
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
bool Node::Publish(const std::string &_topic, const ProtoMsg &_msg)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::GetFullyQualifiedName(this->Options().Partition(),
    this->Options().NameSpace(), _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // Topic not advertised before.
  if (this->dataPtr->topicsAdvertised.find(fullyQualifiedTopic) ==
      this->dataPtr->topicsAdvertised.end())
  {
    return false;
  }

  // Check that the msg type matches the type previously advertised
  // for topic '_topic'.
  MessagePublisher pub;
  auto &info = this->dataPtr->shared->discovery->DiscoveryMsgInfo();
  std::string procUuid = this->dataPtr->shared->pUuid;
  std::string nodeUuid = this->dataPtr->nUuid;
  if (!info.GetPublisher(fullyQualifiedTopic, procUuid, nodeUuid, pub))
  {
    std::cerr << "Node::Publish() I cannot find the msgType registered for "
              << "topic [" << _topic << "]" << std::endl;
    return false;
  }

  if (pub.MsgTypeName() != _msg.GetTypeName())
  {
    std::cerr << "Node::Publish() Type mismatch." << std::endl
              << "\t* Type advertised: " << pub.MsgTypeName() << std::endl
              << "\t* Type published: " << _msg.GetTypeName() << std::endl;
    return false;
  }

  // Local subscribers.
  std::map<std::string, ISubscriptionHandler_M> handlers;
  if (this->dataPtr->shared->localSubscriptions.GetHandlers(fullyQualifiedTopic,
        handlers))
  {
    for (auto &node : handlers)
    {
      for (auto &handler : node.second)
      {
        ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;

        if (subscriptionHandlerPtr)
        {
          if (subscriptionHandlerPtr->GetTypeName() != _msg.GetTypeName())
            continue;

          subscriptionHandlerPtr->RunLocalCallback(_msg);
        }
        else
        {
          std::cerr << "Node::Publish(): Subscription handler is NULL"
                    << std::endl;
        }
      }
    }
  }

  // Remote subscribers.
  if (this->dataPtr->shared->remoteSubscribers.HasTopic(fullyQualifiedTopic))
  {
    std::string data;
    if (!_msg.SerializeToString(&data))
    {
      std::cerr << "Node::Publish(): Error serializing data" << std::endl;
      return false;
    }

    this->dataPtr->shared->Publish(fullyQualifiedTopic, data,
      _msg.GetTypeName());
  }
  // Debug output.
  // else
  //   std::cout << "There are no remote subscribers...SKIP" << std::endl;

  return true;
}

//////////////////////////////////////////////////
std::vector<std::string> Node::SubscribedTopics() const
{
  std::vector<std::string> v;

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  // I'm a real subscriber if I have interest in a topic and I know a publisher.
  for (auto topic : this->dataPtr->topicsSubscribed)
  {
    // Remove the partition information from the topic.
    topic.erase(0, topic.find_last_of("@") + 1);
    v.push_back(topic);
  }

  return v;
}

//////////////////////////////////////////////////
bool Node::Unsubscribe(const std::string &_topic)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::GetFullyQualifiedName(this->Options().Partition(),
    this->Options().NameSpace(), _topic, fullyQualifiedTopic))
  {
    std::cerr << "Topic [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock(this->Shared()->discovery->Mutex(), this->dataPtr->shared->mutex);
  std::lock_guard<std::recursive_mutex> discLk(
    this->Shared()->discovery->Mutex(), std::adopt_lock);
  std::lock_guard<std::recursive_mutex> lk(
    this->dataPtr->shared->mutex, std::adopt_lock);

  this->dataPtr->shared->localSubscriptions.RemoveHandlersForNode(
    fullyQualifiedTopic, this->dataPtr->nUuid);

  // Remove the topic from the list of subscribed topics in this node.
  this->dataPtr->topicsSubscribed.erase(fullyQualifiedTopic);

  // Remove the filter for this topic if I am the last subscriber.
  if (!this->dataPtr->shared->localSubscriptions.HasHandlersForTopic(
    fullyQualifiedTopic))
  {
    this->dataPtr->shared->subscriber->setsockopt(
      ZMQ_UNSUBSCRIBE, fullyQualifiedTopic.data(), fullyQualifiedTopic.size());
  }

  // Notify to the publishers that I am no longer interested in the topic.
  MsgAddresses_M addresses;
  if (!this->dataPtr->shared->discovery->MsgPublishers(fullyQualifiedTopic,
    addresses))
  {
    return false;
  }

  for (auto &proc : addresses)
  {
    for (auto &node : proc.second)
    {
      zmq::socket_t socket(*this->dataPtr->shared->context, ZMQ_DEALER);

      // Set ZMQ_LINGER to 0 means no linger period. Pending messages will be
      // discarded immediately when the socket is closed. That avoids infinite
      // waits if the publisher is disconnected.
      int lingerVal = 200;
      socket.setsockopt(ZMQ_LINGER, &lingerVal, sizeof(lingerVal));

      socket.connect(node.Ctrl().c_str());

      zmq::message_t msg;
      msg.rebuild(fullyQualifiedTopic.size());
      memcpy(msg.data(), fullyQualifiedTopic.data(),
        fullyQualifiedTopic.size());
      socket.send(msg, ZMQ_SNDMORE);

      msg.rebuild(this->dataPtr->shared->myAddress.size());
      memcpy(msg.data(), this->dataPtr->shared->myAddress.data(),
             this->dataPtr->shared->myAddress.size());
      socket.send(msg, ZMQ_SNDMORE);

      msg.rebuild(this->dataPtr->nUuid.size());
      memcpy(msg.data(), this->dataPtr->nUuid.data(),
             this->dataPtr->nUuid.size());
      socket.send(msg, ZMQ_SNDMORE);

      std::string data = std::to_string(EndConnection);
      msg.rebuild(data.size());
      memcpy(msg.data(), data.data(), data.size());
      socket.send(msg, 0);
    }
  }

  return true;
}

//////////////////////////////////////////////////
std::vector<std::string> Node::AdvertisedServices() const
{
  std::vector<std::string> v;

  std::lock_guard<std::recursive_mutex> lk(this->dataPtr->shared->mutex);

  for (auto service : this->dataPtr->srvsAdvertised)
  {
    // Remove the partition information from the service name.
    service.erase(0, service.find_last_of("@") + 1);
    v.push_back(service);
  }

  return v;
}

//////////////////////////////////////////////////
bool Node::UnadvertiseSrv(const std::string &_topic)
{
  std::string fullyQualifiedTopic;
  if (!TopicUtils::GetFullyQualifiedName(this->Options().Partition(),
    this->Options().NameSpace(), _topic, fullyQualifiedTopic))
  {
    std::cerr << "Service [" << _topic << "] is not valid." << std::endl;
    return false;
  }

  std::lock(this->Shared()->discovery->Mutex(), this->dataPtr->shared->mutex);
  std::lock_guard<std::recursive_mutex> discLk(
    this->Shared()->discovery->Mutex(), std::adopt_lock);
  std::lock_guard<std::recursive_mutex> lk(
    this->dataPtr->shared->mutex, std::adopt_lock);

  // Remove the topic from the list of advertised topics in this node.
  this->dataPtr->srvsAdvertised.erase(fullyQualifiedTopic);

  // Remove all the REP handlers for this node.
  this->dataPtr->shared->repliers.RemoveHandlersForNode(
    fullyQualifiedTopic, this->dataPtr->nUuid);

  // Notify the discovery service to unregister and unadvertise my services.
  if (!this->dataPtr->shared->discovery->UnadvertiseSrv(fullyQualifiedTopic,
    this->dataPtr->nUuid))
  {
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
void Node::TopicList(std::vector<std::string> &_topics) const
{
  std::vector<std::string> allTopics;
  _topics.clear();

  std::lock(this->Shared()->discovery->Mutex(), this->dataPtr->shared->mutex);
  std::lock_guard<std::recursive_mutex> discLk(
    this->Shared()->discovery->Mutex(), std::adopt_lock);
  std::lock_guard<std::recursive_mutex> lk(
    this->dataPtr->shared->mutex, std::adopt_lock);

  this->dataPtr->shared->discovery->TopicList(allTopics);

  for (auto &topic : allTopics)
  {
    // Get the partition name.
    std::string partition = topic.substr(1, topic.find_last_of("@") - 1);
    // Remove the front '/'
    if (!partition.empty())
      partition.erase(partition.begin());

    // Discard if the partition name does not match this node's partition.
    if (partition != this->Options().Partition())
      continue;

    // Remove the partition part from the topic.
    topic.erase(0, topic.find_last_of("@") + 1);

    _topics.push_back(topic);
  }
}

//////////////////////////////////////////////////
void Node::ServiceList(std::vector<std::string> &_services) const
{
  std::vector<std::string> allServices;
  _services.clear();

  std::lock(this->Shared()->discovery->Mutex(), this->dataPtr->shared->mutex);
  std::lock_guard<std::recursive_mutex> discLk(
    this->Shared()->discovery->Mutex(), std::adopt_lock);
  std::lock_guard<std::recursive_mutex> lk(
    this->dataPtr->shared->mutex, std::adopt_lock);

  this->dataPtr->shared->discovery->ServiceList(allServices);

  for (auto &service : allServices)
  {
    // Get the partition name.
    std::string partition = service.substr(1, service.find_last_of("@") - 1);
    // Remove the front '/'
    if (!partition.empty())
      partition.erase(partition.begin());

    // Discard if the partition name does not match this node's partition.
    if (partition != this->Options().Partition())
      continue;

    // Remove the partition part from the service.
    service.erase(0, service.find_last_of("@") + 1);

    _services.push_back(service);
  }
}

//////////////////////////////////////////////////
NodeShared *Node::Shared() const
{
  return this->dataPtr->shared;
}

//////////////////////////////////////////////////
const std::string &Node::NodeUuid() const
{
  return this->dataPtr->nUuid;
}

//////////////////////////////////////////////////
std::unordered_set<std::string> &Node::TopicsAdvertised() const
{
  return this->dataPtr->topicsAdvertised;
}

//////////////////////////////////////////////////
std::unordered_set<std::string> &Node::TopicsSubscribed() const
{
  return this->dataPtr->topicsSubscribed;
}

//////////////////////////////////////////////////
std::unordered_set<std::string> &Node::SrvsAdvertised() const
{
  return this->dataPtr->srvsAdvertised;
}

//////////////////////////////////////////////////
NodeOptions &Node::Options() const
{
  return this->dataPtr->options;
}
