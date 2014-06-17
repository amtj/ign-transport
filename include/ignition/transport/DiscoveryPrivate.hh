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

#ifndef __IGN_TRANSPORT_DISCOVERY_PRIVATE_HH_INCLUDED__
#define __IGN_TRANSPORT_DISCOVERY_PRIVATE_HH_INCLUDED__

#include <czmq.h>
#include <uuid/uuid.h>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Packet.hh"
#include "ignition/transport/TopicStorage.hh"
#include "ignition/transport/TransportTypes.hh"

namespace ignition
{
  namespace transport
  {
    /// \class DiscoveryPrivate DiscoveryPrivate.hh
    /// \brief Private data for the Discovery class.
    class IGNITION_VISIBLE DiscoveryPrivate
    {
      /// \def Timestamp
      /// \brief Used to evaluate the validity of a discovery entry.
      typedef std::chrono::time_point<std::chrono::steady_clock> Timestamp;

      /// \brief Constructor.
      /// \param[in] _pUuid This discovery instance will run inside a
      /// transport process. This parameter is the transport process' UUID.
      /// \param[in] _verbose true for enabling verbose mode.
      public: DiscoveryPrivate(const std::string &_pUuid,
                               bool _verbose);

      /// \brief Destructor.
      public: virtual ~DiscoveryPrivate();

      /// \brief Advertise a new message or service call.
      /// \param[in] _advType Message (Msg) or service call (Srv).
      /// \param[in] _topic Topic name to be advertised.
      /// \param[in] _addr ZeroMQ address of the topic's publisher.
      /// \param[in] _ctrl ZeroMQ control address of the topic's publisher.
      /// \param[in] _nUuid Node UUID.
      /// \param[in] _scope Topic scope.
      public: void Advertise(const MsgType &_advType,
                             const std::string &_topic,
                             const std::string &_addr,
                             const std::string &_ctrl,
                             const std::string &_nUuid,
                             const Scope &_scope);

      /// \brief Request discovery information about a topic.
      /// \param[in] _topic Topic name requested.
      /// \param[in] _isSrvCall true if the topic corresponds to a service call.
      public: void Discover(const std::string &_topic, bool _isSrvCall);

      /// \brief Check the validity of the topic information. Each topic update
      /// has its own timestamp. This method iterates over the list of topics
      /// and invalids the old topics.
      public: void RunActivityTask();

      /// \brief Broadcast periodic heartbeats.
      public: void RunHeartbitTask();

      /// \brief Receive discovery messages.
      public: void RunReceptionTask();

      /// \brief Method in charge of receiving the discovery updates.
      public: void RecvDiscoveryUpdate();

      /// \brief Parse a discovery message received via the UDP broadcast socket
      /// \param[in] _fromIp IP address of the message sender.
      /// \param[in] _msg Received message.
      public: void DispatchDiscoveryMsg(const std::string &_fromIp,
                                        char *_msg);

      /// \brief Broadcast a discovery message.
      /// \param[in] _type Message type.
      /// \param[in] _topic Topic name.
      /// \param[in] _addr 0MQ Address.
      /// \param[in] _ctrl 0MQ control address.
      /// \param[in] _nUuid Node's UUID.
      /// \param[in] _flags Optional flags. Currently, the flags are not used
      /// but they will in the future for specifying things like compression,
      /// or encryption.
      /// \return 0 when success.
      public: int SendMsg(uint8_t _type,
                          const std::string &_topic,
                          const std::string &_addr,
                          const std::string &_ctrl,
                          const std::string &_nUuid,
                          const Scope &_scope,
                          int _flags = 0);

      /// \brief Get the IP address of this host.
      /// \return A string with this host's IP address.
      public: std::string GetHostAddr();

      /// \brief Print the current discovery state (info, activity, unknown).
      public: void PrintCurrentState();

      /// \brief Create a new beacon for a given topic advertised by a node.
      /// \param[in] _advType Used to distinguish between regular pub/sub
      /// messages or service calls.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node UUID of the advertiser.
      public: void NewBeacon(const MsgType &_advType,
                             const std::string &_topic,
                             const std::string &_nUuid);

      /// \brief Delete a beacon.
      /// \param[in] _topic Topic name.
      /// \param[in] _nUuid Node UUID of the advertiser.
      public: void DelBeacon(const std::string &_topic,
                             const std::string &_nUuid);

      /// \brief Default activity interval value (ms.).
      /// \sa GetActivityInterval.
      /// \sa SetActivityInterval.
      public: static const unsigned int DefActivityInterval = 100;

      /// \brief Default heartbit interval value (ms.).
      /// \sa GetHeartbitInterval.
      /// \sa SetHeartbitInterval.
      public: static const unsigned int DefHeartbitInterval = 1000;

      /// \brief Default silence interval value (ms.).
      /// \sa GetMaxSilenceInterval.
      /// \sa SetMaxSilenceInterval.
      public: static const unsigned int DefSilenceInterval = 3000;

      /// \brief Default advertise interval value (ms.).
      /// \sa GetAdvertiseInterval.
      /// \sa SetAdvertiseInterval.
      public: static const unsigned int DefAdvertiseInterval = 1000;

      /// \brief Port used to broadcast the discovery messages.
      public: static const int DiscoveryPort = 11312;

      /// \brief Timeout used for receiving messages (ms.).
      public: static const int Timeout = 250;

      /// \brief Host IP address.
      public: std::string hostAddr;

      /// \brief Process UUID.
      public: std::string pUuid;

      /// \brief Silence interval value (ms.).
      /// \sa GetMaxSilenceInterval.
      /// \sa SetMaxSilenceInterval.
      public: unsigned int silenceInterval;

      /// \brief Activity interval value (ms.).
      /// \sa GetActivityInterval.
      /// \sa SetActivityInterval.
      public: unsigned int activityInterval;

      /// \brief Advertise interval value (ms.).
      /// \sa GetAdvertiseInterval.
      /// \sa SetAdvertiseInterval.
      public: unsigned int advertiseInterval;

      /// \brief Heartbit interval value (ms.).
      /// \sa GetHeartbitInterval.
      /// \sa SetHeartbitInterval.
      public: unsigned int heartbitInterval;

      /// \brief Callback executed when new topics are discovered.
      public: DiscoveryCallback connectionCb;

      /// \brief Callback executed when new topics are invalid.
      public: DiscoveryCallback disconnectionCb;

      /// \brief Callback executed when new service call topics are discovered.
      public: DiscoveryCallback connectionSrvCb;

      /// \brief Callback executed when new service call topics are invalid.
      public: DiscoveryCallback disconnectionSrvCb;

      /// \brief Beacons to advertise topics periodically. The key is the topic
      /// name. The value is another map, where the key is the node UUID.
      public: std::map<std::string, std::map<std::string, zbeacon_t*>> beacons;

      /// \brief Topic addressing information.
      public: TopicStorage info;

      /// \brief Activity information. Every time there is a message from a
      /// remote node, its activity information is updated. If we do not hear
      /// from a node in a while, its entries in 'info' will be invalided. The
      /// key is the process uuid.
      public: std::map<std::string, Timestamp> activity;

      /// \brief Print discovery information to stdout.
      public: bool verbose;

      /// \brief ZMQ context for the discovery beacon.
      public: zctx_t *ctx;

      /// \brief Discovery beacon.
      public: zbeacon_t *beacon;

      /// \brief Mutex to guarantee exclusive access between the threads.
      public: std::mutex mutex;

      /// \brief tTread in charge of receiving and handling incoming messages.
      public: std::thread *threadReception;

      /// \brief Thread in charge of sending heartbits.
      public: std::thread *threadHeartbit;

      /// \brief Thread in charge of update the activity.
      public: std::thread *threadActivity;

      /// \brief Mutex to guarantee exclusive access to the exit variable.
      public: std::mutex exitMutex;

      /// \brief When true, the service threads will finish.
      public: bool exit;
    };
  }
}
#endif
