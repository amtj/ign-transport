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
#include <uuid/uuid.h>
#include <zmq.hpp>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "ignition/transport/NodePrivate.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TopicsInfo.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;

//////////////////////////////////////////////////
transport::NodePrivatePtr transport::NodePrivate::GetInstance(bool _verbose)
{
  static NodePrivatePtr instance(new NodePrivate(_verbose));
  return instance;
}

//////////////////////////////////////////////////
transport::NodePrivate::NodePrivate(bool _verbose)
  : bcastPort(11312),
    context(new zmq::context_t(1)),
    publisher(new zmq::socket_t(*context, ZMQ_PUB)),
    subscriber(new zmq::socket_t(*context, ZMQ_SUB))
{
  char bindEndPoint[1024];

  // Initialize random seed.
  srand(time(nullptr));

  this->verbose = _verbose;

  // msecs.
  this->timeout = 250;

  uuid_generate(this->guid);

  this->guidStr = transport::GetGuidStr(this->guid);

  // Initialize the 0MQ objects.
  try
  {
    // Set broadcast/listen discovery beacon.
    this->ctx = zctx_new();
    this->beacon = zbeacon_new(ctx, this->bcastPort);
    zbeacon_subscribe(this->beacon, NULL, 0);
    zbeacon_set_interval(this->beacon, 2500);

    // Set the hostname's ip address
    this->hostAddr = zbeacon_hostname(this->beacon);

    std::string anyTcpEP = "tcp://" + this->hostAddr + ":*";
    this->publisher->bind(anyTcpEP.c_str());
    size_t size = sizeof(bindEndPoint);
    this->publisher->getsockopt(ZMQ_LAST_ENDPOINT, &bindEndPoint, &size);
    this->myAddress = bindEndPoint;
  }
  catch(const zmq::error_t& ze)
  {
     std::cerr << "Error: " << ze.what() << std::endl;
     std::exit(EXIT_FAILURE);
  }

  if (this->verbose)
  {
    std::cout << "Current host address: " << this->hostAddr << std::endl;
    std::cout << "Bind at: [" << this->myAddress << "] for pub/sub\n";
    std::cout << "GUID: " << this->guidStr << std::endl;
  }

  // We don't want to exit yet.
  this->exitMutex.lock();
  this->exit = false;
  this->exitMutex.unlock();

  // Start the service thread.
  this->threadInbound = new std::thread(&transport::NodePrivate::Spin, this);
}

//////////////////////////////////////////////////
transport::NodePrivate::~NodePrivate()
{
  // Tell the service thread to terminate.
  this->exitMutex.lock();
  this->exit = true;
  this->exitMutex.unlock();

  // Wait for the service thread before exit.
  this->threadInbound->join();

  // Stop listening discovery messages.
  zbeacon_unsubscribe(this->beacon);

  // Stop the beacon broadcasts.
  zbeacon_silence(this->beacon);

  // Destroy the beacon.
  zbeacon_destroy(&this->beacon);
  zctx_destroy(&this->ctx);
}

//////////////////////////////////////////////////
void transport::NodePrivate::SpinOnce()
{
  //  Poll socket for a reply, with timeout
  zmq::pollitem_t items[] = {
    { *this->subscriber, 0, ZMQ_POLLIN, 0 },
    { zbeacon_socket(this->beacon), 0, ZMQ_POLLIN, 0 },
  };
  zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->timeout);

  //  If we got a reply, process it
  if (items[0].revents & ZMQ_POLLIN)
    this->RecvMsgUpdate();
  else if (items[1].revents & ZMQ_POLLIN)
    this->RecvDiscoveryUpdate();
}

//////////////////////////////////////////////////
void transport::NodePrivate::Spin()
{
  while (true)
  {
    this->SpinOnce();

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
int transport::NodePrivate::Publish(const std::string &_topic,
                                    const std::string &_data)
{
  assert(_topic != "");

  std::lock_guard<std::mutex> lock(this->mutex);

  if (this->topics.AdvertisedByMe(_topic))
  {
    zmq::message_t message;
    message.rebuild(_topic.size() + 1);
    memcpy(message.data(), _topic.c_str(), _topic.size() + 1);
    this->publisher->send(message, ZMQ_SNDMORE);

    message.rebuild(this->myAddress.size() + 1);
    memcpy(message.data(), this->myAddress.c_str(),
           this->myAddress.size() + 1);
    this->publisher->send(message, ZMQ_SNDMORE);

    message.rebuild(_data.size() + 1);
    memcpy(message.data(), _data.c_str(), _data.size() + 1);
    this->publisher->send(message, 0);

    return 0;
  }
  else
  {
    if (this->verbose)
      std::cerr << "\nNot published. (" << _topic << ") not advertised\n";
    return -1;
  }
}

//////////////////////////////////////////////////
void transport::NodePrivate::RecvDiscoveryUpdate()
{
  std::cout << "beacon received" << std::endl;

  // Address of datagram source.
  char *srcAddr = zstr_recv(zbeacon_socket(this->beacon));

  // A zmq message.
  zframe_t *frame = zframe_recv(zbeacon_socket(this->beacon));

  // Pointer to the raw discovery data.
  byte *data = zframe_data(frame);

  if (this->verbose)
    std::cout << "\nReceived discovery update from " << srcAddr << std::endl;

  if (this->DispatchDiscoveryMsg(reinterpret_cast<char*>(&data[0])) != 0)
    std::cerr << "Something went wrong parsing a discovery message\n";

  zstr_free(&srcAddr);
  zframe_destroy(&frame);
}

//////////////////////////////////////////////////
void transport::NodePrivate::RecvMsgUpdate()
{
  std::lock_guard<std::mutex> lock(this->mutex);

  zmq::message_t message(0);
  std::string topic;
  // std::string sender;
  std::string data;

  try
  {
    if (!this->subscriber->recv(&message, 0))
      return;
    topic = std::string(reinterpret_cast<char *>(message.data()));

    if (!this->subscriber->recv(&message, 0))
      return;
    // sender = std::string(reinterpret_cast<char *>(message.data()));

    if (!this->subscriber->recv(&message, 0))
      return;
    data = std::string(reinterpret_cast<char *>(message.data()));
  }
  catch(const zmq::error_t &_error)
  {
    std::cout << "Error: " << _error.what() << std::endl;
    return;
  }

  if (this->topics.Subscribed(topic))
  {
    // Execute the callback registered
    transport::ISubscriptionHandler_M handlers;
    this->topics.GetSubscriptionHandlers(topic, handlers);
    for (auto handler : handlers)
    {
      ISubscriptionHandlerPtr subscriptionHandlerPtr = handler.second;
      if (subscriptionHandlerPtr)
        // ToDo(caguero): Unserialize only once.
        subscriptionHandlerPtr->RunCallback(topic, data);
      else
        std::cerr << "Subscription handler is NULL" << std::endl;
    }
  }
  else
    std::cerr << "I am not subscribed to topic [" << topic << "]\n";
}

//////////////////////////////////////////////////
int transport::NodePrivate::DispatchDiscoveryMsg(char *_msg)
{
  std::lock_guard<std::mutex> lock(this->mutex);

  Header header;
  AdvMsg advMsg;
  std::string address;
  char *pBody = _msg;

  header.Unpack(_msg);
  pBody += header.GetHeaderLength();

  std::string topic = header.GetTopic();
  std::string rcvdGuid = transport::GetGuidStr(header.GetGuid());

  if (this->verbose)
    header.Print();

  switch (header.GetType())
  {
    case transport::AdvType:
      // Read the address
      advMsg.UnpackBody(pBody);
      address = advMsg.GetAddress();

      if (this->verbose)
        advMsg.PrintBody();

      // Register the advertised address for the topic
      this->topics.AddAdvAddress(topic, address);

      /*std::cout << "Subscribed? " << this->topics.Subscribed(topic) << std::endl;
      std::cout << "Connected? " << this->topics.Connected(topic) << std::endl;
      std::cout << "GUID? " << this->guidStr.compare(rcvdGuid) << std::endl;*/

      if (this->topics.Subscribed(topic))
      {
        // Add a filter for this topic
        this->subscriber->setsockopt(ZMQ_SUBSCRIBE, topic.data(), topic.size());
      }

      // Check if we are interested in this topic
      if (this->topics.Subscribed(topic) &&
          !this->topics.Connected(topic) &&
          this->guidStr.compare(rcvdGuid) != 0)
      {
        std::cout << "Connecting" << std::endl;
        try
        {
          this->subscriber->connect(address.c_str());
          this->topics.SetConnected(topic, true);
          if (this->verbose)
            std::cout << "\t* Connected to [" << address << "]\n";
        }
        catch(const zmq::error_t& ze)
        {
          std::cout << "Error connecting [" << ze.what() << "]\n";
        }
      }

      break;

    case transport::SubType:
      // Check if I advertise the topic requested
      if (this->topics.AdvertisedByMe(topic))
      {
        // Send to the broadcast socket an ADVERTISE message
        this->SendAdvertiseMsg(transport::AdvType, topic, this->myAddress);

        // It's only considered a remote subscriber if the GUID is not as mine.
        if (this->guidStr.compare(rcvdGuid) != 0)
          this->topics.AddSubscriber(topic);
      }

      break;

    default:
      std::cerr << "Unknown message type [" << header.GetType() << "]\n";
      break;
  }

  return 0;
}

//////////////////////////////////////////////////
int transport::NodePrivate::SendAdvertiseMsg(uint8_t _type,
                                             const std::string &_topic,
                                             const std::string &_address)
{
  assert(_topic != "");

  if (!this->topics.AdvertisedByMe(_topic))
  {
    std::cerr << "Topic (" << _topic << ") not advertised by this node\n";
    return -1;
  }

  if (this->verbose)
  {
    std::cout << "\t* Sending ADV msg [" << _topic << "][" << _address
              << "]" << std::endl;
  }

  // Create the beacon content.
  Header header(transport::Version, this->guid, _topic, _type, 0);
  AdvMsg advMsg(header, _address);
  std::vector<char> buffer(advMsg.GetMsgLength());
  advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));

  // Just send one message.
  zbeacon_publish(this->beacon, reinterpret_cast<unsigned char*>(&buffer[0]),
                  advMsg.GetMsgLength());
  zbeacon_silence(this->beacon);

  return 0;
}

//////////////////////////////////////////////////
int transport::NodePrivate::SendSubscribeMsg(uint8_t _type,
                                             const std::string &_topic)
{
  assert(_topic != "");

  if (this->verbose)
    std::cout << "\t* Sending SUB msg [" << _topic << "]" << std::endl;

  Header header(transport::Version, this->guid, _topic, _type, 0);

  std::vector<char> buffer(header.GetHeaderLength());
  header.Pack(reinterpret_cast<char*>(&buffer[0]));

  // Send the data through the UDP broadcast socket
  zbeacon_publish(this->beacon, reinterpret_cast<unsigned char*>(&buffer[0]),
                  header.GetHeaderLength());

  return 0;
}
