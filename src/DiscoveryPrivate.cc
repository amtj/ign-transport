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
#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "ignition/transport/DiscoveryPrivate.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
DiscoveryPrivate::DiscoveryPrivate(const uuid_t &_procUuid,
  bool _verbose)
  : silenceInterval(DefSilenceInterval),
    activityInterval(DefActivityInterval),
    retransmissionInterval(DefRetransmissionInterval),
    heartbitInterval(DefHeartbitInterval),
    connectionCb(nullptr),
    disconnectionCb(nullptr),
    verbose(_verbose),
    exit(false)
{
  this->ctx = zctx_new();

  // Store the UUID and its string version.
  uuid_copy(this->uuid, _procUuid);
  this->uuidStr = GetGuidStr(this->uuid);

  // Discovery beacon.
  this->beacon = zbeacon_new(this->ctx, this->DiscoveryPort);
  zbeacon_subscribe(this->beacon, NULL, 0);

  // Start the thread that receives discovery information.
  this->threadReception =
    new std::thread(&DiscoveryPrivate::RunReceptionService, this);

  // Start the thread that sends heartbeats.
  this->threadHeartbit =
    new std::thread(&DiscoveryPrivate::RunHeartbitService, this);

  // Start the thread that checks the topic information validity.
  this->threadActivity =
    new std::thread(&DiscoveryPrivate::RunActivityService, this);

  // Start the thread that retransmits the discovery requests.
  this->threadRetransmission =
    new std::thread(&DiscoveryPrivate::RunRetransmissionService, this);

  if (this->verbose)
    this->PrintCurrentState();
}

//////////////////////////////////////////////////
DiscoveryPrivate::~DiscoveryPrivate()
{
  // Tell the service thread to terminate.
  this->exitMutex.lock();
  this->exit = true;
  this->exitMutex.unlock();

  // Wait for the service threads to finish before exit.
  this->threadReception->join();
  this->threadHeartbit->join();
  this->threadActivity->join();
  this->threadRetransmission->join();

  // Broadcast a BYE message to trigger the remote cancellation of
  // all our advertised topics.
  this->SendMsg(ByeType, "", "", "");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  zctx_destroy(&this->ctx);
}

//////////////////////////////////////////////////
void DiscoveryPrivate::RunActivityService()
{
  while (true)
  {
    this->mutex.lock();

    Timestamp now = std::chrono::steady_clock::now();
    for (auto it = this->activity.cbegin(); it != this->activity.cend();)
    {
      // Skip my own entry.
      if (it->first == this->uuidStr)
        continue;

      // Elapsed time since the last update from this publisher.
      std::chrono::duration<double> elapsed = now - it->second;

      // This publisher has expired.
      if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
          > this->silenceInterval)
      {
        // Remove all the info entries for this process UUID.
        this->DelTopicAddress("", "", it->first);

        // Notify without topic information. This is useful to inform the client
        // that a remote node is gone, even if we were not interested in its
        // topics.
        this->disconnectionCb("", "", "", it->first);

        // Remove the activity entry.
        this->activity.erase(it++);
      }
      else
        ++it;
    }
    this->mutex.unlock();

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->activityInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::RunHeartbitService()
{
  while (true)
  {
    this->mutex.lock();
    this->SendMsg(HelloType, "", "", "");
    this->mutex.unlock();

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->heartbitInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::RunReceptionService()
{
  while (true)
  {
    // Poll socket for a reply, with timeout.
    zmq::pollitem_t items[] =
    {
      {zbeacon_socket(this->beacon), 0, ZMQ_POLLIN, 0}
    };
    zmq::poll(&items[0], sizeof(items) / sizeof(items[0]), this->Timeout);

    //  If we got a reply, process it.
    if (items[0].revents & ZMQ_POLLIN)
    {
      this->RecvDiscoveryUpdate();

      if (this->verbose)
        this->PrintCurrentState();
    }

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::RunRetransmissionService()
{
  while (true)
  {
    this->mutex.lock();

    // Iterate over the list of topics that have no discovery information yet.
    for (auto topic : this->unknownTopics)
      this->SendMsg(SubType, topic, "", "");

    this->mutex.unlock();

    std::this_thread::sleep_for(
      std::chrono::milliseconds(this->retransmissionInterval));

    // Is it time to exit?
    {
      std::lock_guard<std::mutex> lock(this->exitMutex);
      if (this->exit)
        break;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::RecvDiscoveryUpdate()
{
  std::lock_guard<std::mutex> lock(this->mutex);

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
int DiscoveryPrivate::DispatchDiscoveryMsg(char *_msg)
{
  Header header;
  AdvMsg advMsg;
  std::string address;
  std::string controlAddress;
  char *pBody = _msg;

  // Create the header from the raw bytes.
  header.Unpack(_msg);
  pBody += header.GetHeaderLength();

  std::string topic = header.GetTopic();
  std::string recvProcUuid = GetGuidStr(header.GetGuid());

  // Discard our own discovery messages.
  if (recvProcUuid == this->uuidStr)
    return 0;

  // Update timestamp.
  this->activity[recvProcUuid] = std::chrono::steady_clock::now();

  if (this->verbose)
    header.Print();

  switch (header.GetType())
  {
    case AdvType:
    {
      // Read the address.
      advMsg.UnpackBody(pBody);
      address = advMsg.GetAddress();
      controlAddress = advMsg.GetControlAddress();

      if (this->verbose)
        advMsg.PrintBody();

      // Register an advertised address for the topic.
      bool added =
        this->AddTopicAddress(topic, address, controlAddress, recvProcUuid);

      // Remove topic from unkown topics.
      this->unknownTopics.erase(std::remove(this->unknownTopics.begin(),
        this->unknownTopics.end(), topic), this->unknownTopics.end());

      if (added && this->connectionCb)
      {
        // Execute the client's callback.
        this->connectionCb(topic, address, controlAddress, recvProcUuid);
      }
      break;
    }
    case SubType:
    {
      // Check if at least one of my nodes advertises the topic requested.
      if (this->AdvertisedByProc(topic, this->uuidStr))
      {
        for (auto nodeInfo : this->info[topic][this->uuidStr])
        {
          // Answer an ADVERTISE message.
          this->SendMsg(AdvType, topic, nodeInfo.addr, nodeInfo.ctrl);
        }
      }

      break;
    }
    case HelloType:
    {
      // The timestamp has already been updated.
      break;
    }
    case ByeType:
    {
      // Remove the activity entry for this publisher.
      this->activity.erase(recvProcUuid);

      if (this->disconnectionCb)
      {
        // Notify the new disconnection.
        this->disconnectionCb("", "", "", recvProcUuid);
      }

      // Remove the address entry for this topic.
      this->DelTopicAddress("", "", recvProcUuid);

      break;
    }
    case UnadvType:
    {
      // Read the address.
      advMsg.UnpackBody(pBody);
      address = advMsg.GetAddress();
      controlAddress = advMsg.GetControlAddress();

      if (this->disconnectionCb)
      {
        // Notify the new disconnection.
        this->disconnectionCb(topic, address, controlAddress, recvProcUuid);
      }

      // Remove the address entry for this topic.
      this->DelTopicAddress(topic, address, recvProcUuid);

      break;
    }
    default:
    {
      std::cerr << "Unknown message type [" << header.GetType() << "]\n";
      break;
    }
  }

  return 0;
}

//////////////////////////////////////////////////
int DiscoveryPrivate::SendMsg(uint8_t _type, const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, int _flags)
{
  // Create the header.
  Header header(Version, this->uuid, _topic, _type, _flags);

  switch (_type)
  {
    case AdvType:
    {
      // Create the ADVERTISE message.
      AdvMsg advMsg(header, _addr, _ctrl);

      // Create a buffer and serialize the message.
      std::vector<char> buffer(advMsg.GetMsgLength());
      advMsg.Pack(reinterpret_cast<char*>(&buffer[0]));

      // Broadcast the message.
      zbeacon_publish(this->beacon,
        reinterpret_cast<unsigned char*>(&buffer[0]), advMsg.GetMsgLength());

      break;
    }
    case SubType:
    case UnadvType:
    case HelloType:
    case ByeType:
    {
      // Create a buffer and serialize the message.
      std::vector<char> buffer(header.GetHeaderLength());
      header.Pack(reinterpret_cast<char*>(&buffer[0]));

      // Broadcast the message.
      zbeacon_publish(this->beacon,
        reinterpret_cast<unsigned char*>(&buffer[0]), header.GetHeaderLength());

      break;
    }
    default:
      break;
  }

  zbeacon_silence(this->beacon);

  if (this->verbose)
  {
    std::cout << "\t* Sending " << MsgTypesStr[_type] << " msg [" << _topic
              << "]" << std::endl;
  }

  return 0;
}

//////////////////////////////////////////////////
bool DiscoveryPrivate::AdvertisedByProc(const std::string &_topic,
  const std::string &_uuid)
{
  // I do not have the topic.
  if (this->info.find(_topic) == this->info.end())
    return false;

  return this->info[_topic].find(_uuid) != this->info[_topic].end();
}

//////////////////////////////////////////////////
std::string DiscoveryPrivate::GetHostAddr()
{
  std::lock_guard<std::mutex> lock(this->mutex);
  return zbeacon_hostname(this->beacon);
}

//////////////////////////////////////////////////
bool DiscoveryPrivate::AddTopicAddress(const std::string &_topic,
  const std::string &_addr, const std::string &_ctrl, const std::string &_uuid)
{
  // The topic does not exist
  if (this->info.find(_topic) == this->info.end())
  {
    Addresses_M addresses;
    this->info[_topic] = addresses;
  }

  // Check if the process uuid exists.
  auto &m = this->info[_topic];
  if (m.find(_uuid) != m.end())
  {
    // Check that the {_addr, _ctrl} does not exist.
    auto &v = m[_uuid];
    auto found = std::find_if(v.begin(), v.end(),
      [=](Address_t _addrInfo)
      {
        return _addrInfo.addr == _addr;
      });

    // _addr was already existing, just exit.
    if (found != v.end())
      return false;
  }

  // Add a new address.
  m[_uuid].push_back({_addr, _ctrl});
  return true;
}

//////////////////////////////////////////////////
void DiscoveryPrivate::DelTopicAddress(const std::string &/*_topic*/,
  const std::string &_addr, const std::string &_uuid)
{
  for (auto topicInfo : this->info)
  {
    auto m = topicInfo.second;
    for (auto it = m.begin(); it != m.end();)
    {
      auto &v = it->second;
      v.erase(std::remove_if(v.begin(), v.end(), [=](Address_t _addrInfo)
      {
        return _addrInfo.addr == _addr;
      }), v.end());

      it->second = v;

      if (v.empty() || it->first == _uuid)
        m.erase(it++);
      else
        ++it;
    }
  }
}

//////////////////////////////////////////////////
void DiscoveryPrivate::PrintCurrentState()
{
  std::lock_guard<std::mutex> lock(this->mutex);

  std::cout << "---------------" << std::endl;
  std::cout << "Discovery state" << std::endl;
  std::cout << "\tUUID: " << this->uuidStr << std::endl;
  std::cout << "Settings" << std::endl;
  std::cout << "\tActivity: " << this->activityInterval << " ms." << std::endl;
  std::cout << "\tHeartbit: " << this->heartbitInterval << " ms." << std::endl;
  std::cout << "\tRetrans.: " << this->retransmissionInterval << " ms."
    << std::endl;
  std::cout << "\tSilence: " << this->silenceInterval << " ms." << std::endl;
  std::cout << "Known topics" << std::endl;
  if (this->info.empty())
    std::cout << "\t<empty>" << std::endl;
  else
  {
    for (auto topicInfo : this->info)
    {
      std::cout << "\t" << topicInfo.first << std::endl;
      for (auto proc : topicInfo.second)
      {
        std::cout << "\t\tProcess UUID: " << proc.first << std::endl;
        for (auto node : proc.second)
        {
          std::cout << "\t\t\tAddress: " << node.addr << std::endl;
          std::cout << "\t\t\tControl: " << node.ctrl << std::endl;
        }
      }
    }
  }

  // Used to calculate the elapsed time.
  Timestamp now = std::chrono::steady_clock::now();

  std::cout << "Activity" << std::endl;
  if (this->activity.empty())
    std::cout << "\t<empty>" << std::endl;
  else
  {
    for (auto proc : this->activity)
    {
      // Elapsed time since the last update from this publisher.
      std::chrono::duration<double> elapsed = now - proc.second;

      std::cout << "\t" << proc.first << std::endl;
      std::cout << "\t\t" << "Since: " << std::chrono::duration_cast<
        std::chrono::milliseconds>(elapsed).count() << " ms. ago. "
        << std::endl;
    }
  }
  std::cout << "Unknown topics" << std::endl;
  if (this->unknownTopics.empty())
    std::cout << "\t<empty>" << std::endl;
  else
  {
    for (auto topic : this->unknownTopics)
      std::cout << "\t" << topic << std::endl;
  }
  std::cout << "---------------" << std::endl;
}
