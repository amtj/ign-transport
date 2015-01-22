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

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include "ignition/transport/ign.hh"
#include "ignition/transport/Node.hh"
#include "ignition/transport/NodeShared.hh"
#include "ignition/transport/TransportTypes.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE void cmdTopicInfo(char *_topic)
{
  std::cout << "Running ign topic -i " << _topic << std::endl;

  NodeShared *shared = NodeShared::GetInstance();

  // Give the node some time to receive topic updates.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  std::vector<std::string> allTopics;
  shared->discovery->GetTopicList(allTopics);

  for (auto &topic : allTopics)
  {
    std::cout << topic << std::endl;
  }


  MsgAddresses_M pubs;
  std::string topic = "@@" + std::string(_topic);
  std::cout << "Topic: [" << topic << "]" << std::endl;
  shared->discovery->GetMsgPublishers(topic, pubs);
  for (auto &proc : pubs)
    for (auto pub : proc.second)
    {
      std::cout << pub.Addr() << std::endl;
      std::cout << pub.MsgTypeName() << std::endl;
    }
}

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE void cmdTopicList()
{
  Node node;

  // Give the node some time to receive topic updates.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  std::vector<std::string> topics;
  node.GetTopicList(topics);

  for (auto const &topic : topics)
    std::cout << topic << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE void cmdServiceInfo(char *_service)
{
  std::cout << "Running ign service -i " << _service << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE void cmdServiceList()
{
  Node node;

  // Give the node some time to receive topic updates.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  std::vector<std::string> services;
  node.GetServiceList(services);

  for (auto const &service : services)
    std::cout << service << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_VISIBLE char *ignitionVersion()
{
  int majorVersion = IGNITION_TRANSPORT_MAJOR_VERSION;
  int minorVersion = IGNITION_TRANSPORT_MINOR_VERSION;
  int patchVersion = IGNITION_TRANSPORT_PATCH_VERSION;

  return strdup((std::to_string(majorVersion) + "." +
                 std::to_string(minorVersion) + "." +
                 std::to_string(patchVersion)).c_str());
}
