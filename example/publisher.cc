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
#include <csignal>
#include <ignition/transport.hh>
#include "msg/benchmark.pb.h"
#include "msg/stringmsg.pb.h"

bool terminatePub = false;

//////////////////////////////////////////////////
/// \brief Function callback executed when a SIGINT or SIGTERM signals are
/// captured. This is used to break the infinite loop that publishes messages
/// and exit the program smoothly.
void signal_handler(int _signal)
{
  if (_signal == SIGINT || _signal == SIGTERM)
    terminatePub = true;
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Install a signal handler for SIGINT.
  std::signal(SIGINT, signal_handler);

  // Create a transport node and advertise a topic.
  ignition::transport::Node node;
  node.Advertise("/foo");

  // Prepare the message.
  //example::mymsgs::StringMsg msg;
  //msg.set_data("HELLO");

  // Prepare the message.
  example::mymsgs::BenchmarkMsg msg;

  // int numBlocks = exp2(10) / 4; // 1 KB.
  int numBlocks = 1 * exp2(20) / 4; // 100 MB.

  for (auto i = 0; i < numBlocks; ++i)
    msg.add_data(1);


  // Publish messages at 1Hz.
  while(!terminatePub)
  {
    node.Publish("/foo", msg);

    //std::cout << "Publishing hello\n";
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  return 0;
}
